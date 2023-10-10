

#include "graphics-impl.h"
#include <GLFW/glfw3.h>

#include <iostream>
#include <cstdlib>
#include <chrono>


#include <lib/os/msg.h>
#include <lib/os/time.h>
#include <lib/math/math.h>
#include <lib/image/color.h>
#include <lib/image/image.h>
#include <lib/kaba/kaba.h>

#include <lib/hui_minimal/hui.h>

#include "helper/DeletionQueue.h"
#include "helper/PerformanceMonitor.h"
#include "helper/ErrorHandler.h"
#include "helper/Scheduler.h"
#include "helper/ResourceManager.h"

#include "audio/Sound.h"

#include "input/InputManager.h"
#include "input/Keyboard.h"

#include "net/NetworkManager.h"

#include "fx/ParticleManager.h"

#include "gui/gui.h"
#include "gui/Picture.h"
#include "gui/Text.h"

#include "y/EngineData.h"
#include "y/ComponentManager.h"
#include "meta.h"


#include "plugins/PluginManager.h"
#include "plugins/Controller.h"

#include "renderer/base.h"
#include "renderer/world/WorldRenderer.h"
#ifdef USING_VULKAN
	#include "renderer/world/WorldRendererVulkan.h"
	#include "renderer/world/WorldRendererVulkanForward.h"
	#include "renderer/world/WorldRendererVulkanRayTracing.h"
	#include "renderer/gui/GuiRendererVulkan.h"
	#include "renderer/post/HDRRendererVulkan.h"
	#include "renderer/post/PostProcessorVulkan.h"
	#include "renderer/target/WindowRendererVulkan.h"
#else
	#include "renderer/world/WorldRendererGL.h"
	#include "renderer/world/WorldRendererGLForward.h"
	#include "renderer/world/WorldRendererGLDeferred.h"
	#include "renderer/gui/GuiRendererGL.h"
	#include "renderer/post/HDRRendererGL.h"
	#include "renderer/post/PostProcessorGL.h"
	#include "renderer/regions/RegionRendererGL.h"
	#include "renderer/target/WindowRendererGL.h"
using RegionRenderer = RegionRendererGL;
#endif

#include "Config.h"
#include "world/Camera.h"
#include "world/Light.h"
#include "world/Material.h"
#include "world/Model.h"
#include "world/Object.h"
#include "world/Terrain.h"
#include "world/World.h"

const string app_name = "y";
const string app_version = "0.1.0";



// pipeline: shader + z buffer / blending parameters

// descriptor set: textures + shader uniform buffers

void DrawSplashScreen(const string &str, float per) {
	std::cerr << " - splash - " << str.c_str() << "\n";
}

void ExternalModelCleanup(Model *m) {}


using namespace std::chrono;



rect dynamicly_scaled_area(FrameBuffer *fb) {
	//return rect(0, fb->width, 0, fb->height);
	return rect(0, fb->width * engine.resolution_scale_x, 0, fb->height * engine.resolution_scale_y);
}

rect dynamicly_scaled_source() {
	return rect(0, engine.resolution_scale_x, 0, engine.resolution_scale_y);
}


class YEngineApp {
public:
	
	void run(const Array<string> &arg) {
		init(arg);
		load_first_world();
		main_loop();
		cleanup();
	}

//private:
	GLFWwindow* window;

	WorldRenderer *world_renderer = nullptr;
	Renderer *gui_renderer = nullptr;
	RegionRenderer *region_renderer = nullptr;
	PostProcessorStage *hdr_renderer = nullptr;
	PostProcessor *post_processor = nullptr;
	TargetRenderer *renderer = nullptr;

	gui::Text *fps_display;
	int ch_iter = -1;

	string render_graph_str(Renderer *r) {
		string s = PerformanceMonitor::get_name(r->channel);
		if (r->children.num == 1)
			s += " <<< " + render_graph_str(r->children[0]);
		if (r->children.num >= 2) {
			Array<string> ss;
			for (auto c: r->children)
				ss.add(render_graph_str(c));
			s += " <<< (" + implode(ss, ", ") + ")";
		}
		return s;
	}

	void print_render_chain() {
		msg_write("------------------------------------------");
		msg_write("CHAIN:  " + render_graph_str(renderer));
		msg_write("------------------------------------------");
	}

	TargetRenderer *create_window_renderer() {
#ifdef USING_VULKAN
		return new WindowRendererVulkan(window, engine.width, engine.height, device);
#else
		return new WindowRendererGL(window, engine.width, engine.height);
#endif
	}

	Renderer *create_gui_renderer(Renderer *parent) {
#ifdef USING_VULKAN
		return new GuiRendererVulkan(parent);
#else
		return new GuiRendererGL(parent);
#endif
	}

	RegionRenderer *create_region_renderer(Renderer *parent) {
#ifdef USING_VULKAN
		return new RegionRendererVulkan(parent);
#else
		return new RegionRendererGL(parent);
#endif
	}

	PostProcessorStage *create_hdr_renderer(PostProcessor *parent) {
#ifdef USING_VULKAN
		return new HDRRendererVulkan(parent);
#else
		return new HDRRendererGL(parent);
#endif
	}

	PostProcessor *create_post_processor(Renderer *parent) {
#ifdef USING_VULKAN
		return new PostProcessorVulkan(parent);
#else
		return new PostProcessorGL(parent);
#endif
	}

	WorldRenderer *create_world_renderer(Renderer *parent) {
#ifdef USING_VULKAN
		if (config.get_str("renderer.path", "forward") == "raytracing")
			return new WorldRendererVulkanRayTracing(parent, device);
		else
			return new WorldRendererVulkanForward(parent, device);
#else
		if (config.get_str("renderer.path", "forward") == "deferred")
			return new WorldRendererGLDeferred(parent);
		else
			return new WorldRendererGLForward(parent);
#endif
	}

	void create_full_renderer() {
		try {
			engine.window_renderer = renderer = create_window_renderer();
			engine.gui_renderer = gui_renderer = create_gui_renderer(renderer);
			engine.region_renderer = region_renderer = create_region_renderer(gui_renderer);
			if (config.get_str("renderer.path", "forward") == "direct") {
				engine.world_renderer = world_renderer = create_world_renderer(region_renderer->add_region(rect::ID));
			} else {
				engine.post_processor = post_processor = create_post_processor(region_renderer->add_region(rect::ID));
				engine.hdr_renderer = hdr_renderer = create_hdr_renderer(post_processor);
				engine.world_renderer = world_renderer = create_world_renderer(hdr_renderer);
				//post_processor->set_hdr(hdr_renderer);
			}
		} catch(Exception &e) {
			hui::ShowError(e.message());
			throw e;
		}
		print_render_chain();
	}

	void init(const Array<string> &arg) {
		config.load(arg);

		window = create_window();

		kaba::init();
		NetworkManager::init();
		ch_iter = PerformanceMonitor::create_channel("iter");
		ComponentManager::init();
		Scheduler::init(ch_iter);

		engine.app_name = app_name;
		engine.version = app_version;
		if (config.get_str("error.missing-files", "ignore") == "ignore")
			engine.ignore_missing_files = true;



		msg_write("on init...");

		engine.set_dirs(config.game_dir | "Textures",
			config.game_dir | "Maps",
			config.game_dir | "Objects",
			config.game_dir | "Sounds",
			config.game_dir | "Scripts",
			config.game_dir | "Materials",
			config.game_dir | "Fonts");

		auto context = api_init(window);
		auto resource_manager = new ResourceManager(context);
		engine.set_context(context, resource_manager);

		create_full_renderer();

		audio::init();

		GodInit(ch_iter);
		PluginManager::init(ch_iter);

		ErrorHandler::init();

		gui::init(ch_iter);


		input::init(window);
	}

	void load_first_world() {
		msg_error("FIRST WORLD...." + config.default_world);
		if (config.default_world == "")
			throw Exception("no default world defined in game.ini");
		load_world(config.default_world);
	}

	void load_world(const Path &filename) {
		msg_write("o------------------------------------------------------o");
		msg_write("| loading                                              |");
		msg_right();
		reset_game();

		GodLoadWorld(filename);

		fps_display = new gui::Text("", 0.020f, {0.01f, 0.01f});
		fps_display->dz = 900;
		gui::toplevel->add(fps_display);

		for (auto &s: world.scripts)
			PluginManager::add_controller(s.filename, s.variables);
		for (auto &s: config.additional_scripts)
			PluginManager::add_controller(s, {});

		msg_left();
		msg_write("|                                                      |");
		msg_write("o------------------------------------------------------o");
	}
	
	GLFWwindow* create_window() {
		glfwInit();
#ifdef USING_VULKAN
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#else
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
#endif

		int w = config.get_int("screen.width", 1024);
		int h = config.get_int("screen.height", 768);
		engine.width = w;
		engine.height = h;
		auto monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* vidmode = glfwGetVideoMode(monitor);

		string mode = config.get_str("screen.mode", "window");
		bool fullscreen = (mode == "fullscreen"); //config.get_bool("screen.fullscreen", false);
		bool windowed_fullscreen = (mode == "windowed-fullscreen"); //config.get_bool("screen.windowed-fullscreen", false);
		engine.physical_aspect_ratio = (float)w / (float)h;

		glfwWindowHint(GLFW_RED_BITS, vidmode->redBits);
		glfwWindowHint(GLFW_GREEN_BITS, vidmode->greenBits);
		glfwWindowHint(GLFW_BLUE_BITS, vidmode->blueBits);
		glfwWindowHint(GLFW_REFRESH_RATE, vidmode->refreshRate);

		glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);

		if (!fullscreen and !windowed_fullscreen)
			monitor = nullptr;

		if (windowed_fullscreen) {
			w = vidmode->width;
			h = vidmode->height;
		}
		GLFWwindow* window = glfwCreateWindow(w, h, "y-engine", monitor, nullptr);

		glfwSetWindowUserPointer(window, this);
		glfwMakeContextCurrent(window);
		if (config.get_bool("renderer.uncapped-framerate", false))
			glfwSwapInterval(0);
		else
			glfwSwapInterval(1);
		return window;
	}

	void main_loop() {
		while (!glfwWindowShouldClose(window)) {
			PerformanceMonitor::next_frame();
			engine.elapsed_rt = PerformanceMonitor::frame_dt;
			engine.elapsed = engine.time_scale * min(engine.elapsed_rt, 1.0f / config.min_framerate);

			input::iterate();
			Scheduler::handle_input();

			iterate();
			draw_frame();

			if (input::get_key(hui::KEY_CONTROL) and input::get_key(hui::KEY_Q))
				break;

			if (world.next_filename) {
				load_world(world.next_filename);
				world.next_filename = "";
			}
		}

#ifdef USING_VULKAN
		device->wait_idle();
#endif
	}

	void reset_game() {
		world_renderer->reset();
		PluginManager::reset();
		CameraReset();
		world.reset();
		gui::reset();
	}

	void cleanup() {
		reset_game();
		GodEnd();

		delete world_renderer;
		delete renderer;
		api_end();

		glfwDestroyWindow(window);

		glfwTerminate();
	}

	void iterate() {
		PerformanceMonitor::begin(ch_iter);
		Scheduler::handle_iterate_pre(engine.elapsed);

		network_manager.iterate();

		world.iterate(engine.elapsed);
		DeletionQueue::delete_all();

		Scheduler::handle_iterate(engine.elapsed);

		world.particle_manager->iterate(engine.elapsed);
		gui::iterate(engine.elapsed);

		world.iterate_animations(engine.elapsed);
		PerformanceMonitor::end(ch_iter);
	}







	void update_statistics() {
#ifdef USING_VULKAN
		device->wait_idle();
#endif
		fps_display->set_text(format("%.1f", 1.0f / PerformanceMonitor::avg_frame_time));
		fps_display->visible = (config.debug_level >= 1);
	}

	static Array<float> render_times;
	os::Timer timer_render;

	void update_dynamic_resolution() {
		static float drt_time = 0;
		static int drt_frames = 0;
		drt_time += engine.elapsed;
		drt_frames ++;
		if ((drt_time > 0.2f) and (drt_frames > 10)) {
			for (int i=0; i<render_times.num; i++)
				for (int k=i+1; k<render_times.num; k++)
					if (render_times[i] < render_times[k])
						render_times.swap(i, k);
			float dt = render_times[render_times.num / 5];
			float target_dt = 1.0f / config.target_framerate;
			//msg_write(format("%s  ---  %f", fa2s(render_times), target_dt));
			// TODO measure render time better... even when rendering faster than 60Hz OpenGL will wait...
			if (dt > target_dt * 1.05f) {
				engine.resolution_scale_x /= 1.20f;
			} else if (dt < target_dt * 0.82f) {
				engine.resolution_scale_x *= 1.20f;
			}
			engine.resolution_scale_x = clamp(engine.resolution_scale_x, config.resolution_scale_min, config.resolution_scale_max);
			if (engine.resolution_scale_x != engine.resolution_scale_y)
				msg_write(format("dyn res   %.0f %%", engine.resolution_scale_x * 100));
			engine.resolution_scale_y = engine.resolution_scale_x;
			drt_time = 0;
			drt_frames = 0;
			render_times.clear();
		}
	}


	void draw_frame() {
		if (PerformanceMonitor::frames == 0)
			update_statistics();

		update_dynamic_resolution();

		if (!renderer->start_frame())
			return;
		Scheduler::handle_draw_pre();
		timer_render.peek();
		renderer->draw();
		render_times.add(timer_render.get());
		renderer->end_frame();
	}


};


Array<float> YEngineApp::render_times;


YEngineApp app;



int hui_main(const Array<string> &arg) {

	hui::Application::guess_directories(arg, "y");

	try {
		app.run(arg);
	} catch (const std::exception& e) {
		hui::ShowError(e.what());
		return EXIT_FAILURE;
	} catch (const Exception& e) {
		hui::ShowError(e.message());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
