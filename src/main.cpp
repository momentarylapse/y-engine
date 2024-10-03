

#include "graphics-impl.h"
#include <GLFW/glfw3.h>

#include <iostream>
#include <cstdlib>
#include <chrono>


#include <lib/os/msg.h>
#include <lib/os/time.h>
#include <lib/math/math.h>
#include <lib/kaba/kaba.h>

#include <lib/hui_minimal/hui.h>

#include "helper/DeletionQueue.h"
#include "helper/PerformanceMonitor.h"
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
#include "meta.h"


#include "plugins/PluginManager.h"

#include "renderer/base.h"
#include "renderer/helper/RendererFactory.h"
#include "renderer/world/WorldRenderer.h"
#ifdef USING_VULKAN
	#include "renderer/target/WindowRendererVulkan.h"
#else
	#include "renderer/target/WindowRendererGL.h"
#endif

#include "Config.h"
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

	int ch_iter = -1;

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

		create_full_renderer(window, nullptr); // cam_main does not exist yet

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
		audio::attach_listener(cam_main->owner);

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
			gpu_flush();
			PerformanceMonitor::next_frame();
			reset_gpu_timestamp_queries();
#ifdef USING_OPENGL
			gpu_timestamp(-1);
#endif
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
			auto tt = gpu_read_timestamps();
			for (int i=0; i<tt.num; i++)
				PerformanceMonitor::current_frame_timing.gpu.add({gpu_timestamp_queries[i], tt[i]});

		}

		gpu_flush();
	}

	void reset_game() {
		engine.world_renderer->reset();
		PluginManager::reset();
		CameraReset();
		world.reset();
		gui::reset();
	}

	void cleanup() {
		reset_game();
		GodEnd();

		input::remove(window);
		gpu_flush();
		// sometimes there is a weird crash in another thread (I don't know in which library) otherwise
		os::sleep(0.25f);

		delete engine.world_renderer;
		delete engine.window_renderer;
		api_end();
		glfwDestroyWindow(window);

		glfwTerminate();
	}

	void iterate() {
		PerformanceMonitor::begin(ch_iter);
		Scheduler::handle_iterate_pre(engine.elapsed);

		network_manager.iterate();

		world.iterate(engine.elapsed);
		audio::iterate(engine.elapsed);
		DeletionQueue::delete_all();

		Scheduler::handle_iterate(engine.elapsed);

		world.particle_manager->iterate(engine.elapsed);
		gui::iterate(engine.elapsed);

		world.iterate_animations(engine.elapsed);
		PerformanceMonitor::end(ch_iter);
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
		Scheduler::handle_draw_pre();
		timer_render.peek();
		engine.window_renderer->draw(engine.window_renderer->create_params(engine.physical_aspect_ratio));
		render_times.add(timer_render.get());
		engine.window_renderer->end_frame();
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
