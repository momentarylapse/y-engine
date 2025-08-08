

#include <lib/ygraphics/graphics-impl.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <cstdlib>
#include <chrono>


#include <lib/os/msg.h>
#include <lib/os/time.h>
#include <lib/math/math.h>
#include <lib/kaba/kaba.h>
#include <lib/profiler/Profiler.h>

#include "helper/DeletionQueue.h"
#include "helper/ErrorHandler.h"
#include "helper/Scheduler.h"
#include "helper/ResourceManager.h"

#include "audio/audio.h"

#include "input/InputManager.h"
#include "input/Keyboard.h"

#include "net/NetworkManager.h"

#include "fx/ParticleManager.h"

#include "gui/gui.h"

#include "y/EngineData.h"
#include "y/ComponentManager.h"
#include "y/SystemManager.h"
#include "meta.h"


#include "plugins/PluginManager.h"

#include <lib/yrenderer/Context.h>
#include "renderer/helper/RendererFactory.h"
#include "renderer/world/WorldRenderer.h"
#include <lib/yrenderer/target/WindowRenderer.h>
#include "renderer/FullCameraRenderer.h"

#include "Config.h"
#include "lib/os/app.h"
#include "world/Camera.h"
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



namespace yrenderer {
	rect dynamicly_scaled_area(ygfx::FrameBuffer *fb) {
		//return rect(0, fb->width, 0, fb->height);
		return rect(0, fb->width * engine.resolution_scale_x, 0, fb->height * engine.resolution_scale_y);
	}

	rect dynamicly_scaled_source() {
		return rect(0, engine.resolution_scale_x, 0, engine.resolution_scale_y);
	}
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

	int ch_iter = -1;

	void init(const Array<string> &arg) {
		config.load(arg);

		window = create_window();

		kaba::init();
		NetworkManager::init();
		ch_iter = profiler::create_channel("iter");
		ComponentManager::init();
		SchedulerManager::init(ch_iter);

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

		auto context = yrenderer::api_init_glfw(window);
		auto resource_manager = new ResourceManager(context,
			config.game_dir | "Textures",
			config.game_dir | "Materials",
			config.game_dir | "Materials");
		context->shader_manager = resource_manager->shader_manager;
		context->texture_manager = resource_manager->texture_manager;
		context->material_manager = resource_manager->material_manager;
		engine.set_context(context, resource_manager);

		create_base_renderer(context, window);

		audio::init();

		GodInit(ch_iter);
		PluginManager::init();
		SystemManager::init(ch_iter);

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
		audio::attach_listener(cam_main->owner);

		for (auto& cam: ComponentManager::get_list_family<Camera>())
			create_and_attach_camera_renderer(engine.context, cam);
		for (auto &s: world.systems)
			SystemManager::create(s.filename, s.class_name, s.variables);
		for (auto &s: config.additional_scripts)
			SystemManager::create(s, "", {});

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
		string title = "y-engine";
#ifdef USING_VULKAN
		title += " (vulkan)";
#else
		title += " (gl)";
#endif
		GLFWwindow* window = glfwCreateWindow(w, h, title.c_str(), monitor, nullptr);

		glfwSetWindowUserPointer(window, this);
		glfwMakeContextCurrent(window);
		if (config.get_bool("renderer.uncapped-framerate", false))
			glfwSwapInterval(0);
		else
			glfwSwapInterval(1);
		return window;
	}

	void main_loop() {
		while (!glfwWindowShouldClose(window) and !engine.end_requested) {
			engine.context->gpu_flush();
			profiler::next_frame();
			engine.context->reset_gpu_timestamp_queries();
#ifdef USING_OPENGL
			engine.context->gpu_timestamp({}, -1);
#endif
			engine.elapsed_rt = profiler::frame_dt;
			engine.elapsed = engine.time_scale * min(engine.elapsed_rt, 1.0f / config.min_framerate);

			input::iterate();
			SystemManager::handle_input();

			iterate();
			draw_frame();

			if (input::get_key(input::KEY_CONTROL) and input::get_key(input::KEY_Q))
				break;

			if (world.next_filename) {
				load_world(world.next_filename);
				world.next_filename = "";
			}
			auto tt = engine.context->gpu_read_timestamps();
			for (int i=0; i<tt.num; i++)
				profiler::current_frame_timing.gpu.add({engine.context->gpu_timestamp_queries[i], tt[i]});

		}

		engine.context->gpu_flush();
	}

	void reset_game() {
		//for (auto rp: engine.render_paths)
		//	rp->world_renderer->reset();
		SystemManager::reset();
		SchedulerManager::reset();
		CameraReset();
		world.reset();
		gui::reset();
	}

	void cleanup() {
		reset_game();
		GodEnd();

		input::remove(window);
		engine.context->gpu_flush();
		// sometimes there is a weird crash in another thread (I don't know in which library) otherwise
		os::sleep(0.25f);

		// TODO
		//delete engine.world_renderer;
		delete engine.window_renderer;
		engine.resource_manager->clear();
		yrenderer::api_end(engine.context);
		glfwDestroyWindow(window);

		glfwTerminate();
	}

	void iterate() {
		profiler::begin(ch_iter);
		SystemManager::handle_iterate_pre(engine.elapsed);

		network_manager.iterate();

		world.iterate(engine.elapsed);
		audio::iterate(engine.elapsed);
		DeletionQueue::delete_all();

		SystemManager::handle_iterate(engine.elapsed);
		SchedulerManager::iterate(engine.elapsed);
		ComponentManager::iterate(engine.elapsed);

		world.particle_manager->iterate(engine.elapsed);
		gui::iterate(engine.elapsed);

		world.iterate_animations(engine.elapsed);
		profiler::end(ch_iter);
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
		update_dynamic_resolution();

		if (!engine.window_renderer->start_frame())
			return;
		const auto params = engine.window_renderer->create_params(engine.physical_aspect_ratio);
		SystemManager::handle_draw_pre();
		timer_render.peek();
		for (auto t: engine.render_tasks)
			if (t->_priority < 1000 and t->active)
				t->render(params);
		engine.window_renderer->draw(params);
		for (auto t: engine.render_tasks)
			if (t->_priority >= 1000 and t->active)
				t->render(params);
		render_times.add(timer_render.get());
		engine.window_renderer->end_frame(params);
	}


};


Array<float> YEngineApp::render_times;


YEngineApp y_app;


namespace os::app {
int main(const Array<string> &arg) {
	os::app::detect(arg, "y");

	try {
		y_app.run(arg);
	} catch (const std::exception& e) {
		msg_error(e.what());
		return EXIT_FAILURE;
	} catch (const Exception& e) {
		msg_error(e.message());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
}
