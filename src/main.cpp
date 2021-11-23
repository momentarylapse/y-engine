

#include "graphics-impl.h"
#include <GLFW/glfw3.h>

#include <iostream>
#include <cstdlib>
#include <chrono>


#include "lib/math/math.h"
#include "lib/image/color.h"
#include "lib/image/image.h"
#include "lib/kaba/kaba.h"

#ifdef _X_USE_HUI_
	#include "lib/hui/hui.h"
#elif defined(_X_USE_HUI_MINIMAL_)
	#include "lib/hui_minimal/hui.h"
#endif

#include "helper/PerformanceMonitor.h"
#include "helper/ErrorHandler.h"
#include "helper/Scheduler.h"

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
#include "renderer/RenderPath.h"
#ifdef USING_VULKAN
	#include "renderer/RenderPathVulkan.h"
	#include "renderer/RenderPathVulkanForward.h"
	#include "renderer/target/WindowRendererVulkan.h"
#else
	#include "renderer/RenderPathGL.h"
	#include "renderer/RenderPathGLForward.h"
	#include "renderer/RenderPathGLDeferred.h"
	#include "renderer/target/WindowRendererGL.h"
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

	RenderPath *render_path;
	TargetRenderer *renderer;

	gui::Text *fps_display;
	int ch_iter = -1;

	TargetRenderer *create_window_renderer() {
#ifdef USING_VULKAN
		return new WindowRendererVulkan(window, engine.width, engine.height);
#else
		return new WindowRendererGL(window, engine.width, engine.height);
#endif
	}
	RenderPath *create_render_path(Renderer *r) {
#ifdef USING_VULKAN
		return new RenderPathVulkanForward(r, true);
#else
		if (config.get_str("renderer.path", "forward") == "deferred")
			return new RenderPathGLDeferred(r);
		else
			return new RenderPathGLForward(r);
#endif
	}
	void create_full_renderer() {
		try {
			engine.renderer = renderer = create_window_renderer();
			engine.render_path = render_path = create_render_path(renderer);
			renderer->set_child(render_path);
		} catch(Exception &e) {
			hui::ShowError(e.message());
			throw e;
		}
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

		engine.set_dirs("Textures", "Maps", "Objects", "Sounds", "Scripts", "Materials", "Fonts");

		api_init(window);
		create_full_renderer();

		audio::init();

		GodInit(ch_iter);
		PluginManager::init(ch_iter);

		ErrorHandler::init();

		gui::init(ch_iter);


		input::init(window);


		MaterialInit();
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
		for (auto &s: config.get_str("additional-scripts", "").explode(","))
			PluginManager::add_controller(s, {});

		msg_left();
		msg_write("|                                                      |");
		msg_write("o------------------------------------------------------o");
	}
	
	GLFWwindow* create_window() {
		GLFWwindow* window;
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
		const GLFWvidmode* mode = glfwGetVideoMode(monitor);

		bool fullscreen = config.get_bool("screen.fullscreen", false);
		bool windowed_fullscreen = config.get_bool("screen.windowed-fullscreen", false);
		engine.physical_aspect_ratio = (float)w / (float)h;

		glfwWindowHint(GLFW_RED_BITS, mode->redBits);
		glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
		glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
		glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

		if (!fullscreen and !windowed_fullscreen)
			monitor = nullptr;

		if (windowed_fullscreen) {
			w = mode->width;
			h = mode->height;
		}
		window = glfwCreateWindow(w, h, "y-engine", monitor, nullptr);

		glfwSetWindowUserPointer(window, this);
		glfwMakeContextCurrent(window);
		glfwSwapInterval(0);
		return window;
	}

	void main_loop() {
		while (!glfwWindowShouldClose(window)) {
			PerformanceMonitor::next_frame();
			engine.elapsed_rt = PerformanceMonitor::frame_dt;
			engine.elapsed = engine.time_scale * min(engine.elapsed_rt, 0.1f);

			input::iterate();
			Scheduler::handle_input();

			iterate();
			draw_frame();

			if (input::get_key(hui::KEY_CONTROL) and input::get_key(hui::KEY_Q))
				break;

			if (!world.next_filename.is_empty()) {
				load_world(world.next_filename);
				world.next_filename = "";
			}
		}

#ifdef USING_VULKAN
		vulkan::default_device->wait_idle();
#endif
	}

	void reset_game() {
		render_path->reset();
		PluginManager::reset();
		CameraReset();
		world.reset();
		gui::reset();
	}

	void cleanup() {
		reset_game();
		GodEnd();

		delete render_path;
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

		Scheduler::handle_iterate(engine.elapsed);

		world.particle_manager->iterate(engine.elapsed);
		gui::iterate(engine.elapsed);

		world.iterate_animations(engine.elapsed);
		PerformanceMonitor::end(ch_iter);
	}







	void update_statistics() {
#ifdef USING_VULKAN
		vulkan::default_device->wait_idle();
#endif
		fps_display->set_text(format("%.1f", 1.0f / PerformanceMonitor::avg_frame_time));
	}

	static Array<float> render_times;
	hui::Timer timer_render;

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
				render_path->resolution_scale_x /= 1.20f;
			} else if (dt < target_dt * 0.82f) {
				render_path->resolution_scale_x *= 1.20f;
			}
			render_path->resolution_scale_x = clamp(render_path->resolution_scale_x, config.resolution_scale_min, config.resolution_scale_max);
			if (render_path->resolution_scale_x != render_path->resolution_scale_y)
				msg_write(format("dyn res   %.0f %%", render_path->resolution_scale_x * 100));
			render_path->resolution_scale_y = render_path->resolution_scale_x;
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
