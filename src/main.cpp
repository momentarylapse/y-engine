

#include "lib/nix/nix.h"

#include <GLFW/glfw3.h>

#include <iostream>
#include <cstdlib>
#include <chrono>


#include "lib/math/math.h"
#include "lib/image/color.h"
#include "lib/image/image.h"
#include "lib/kaba/kaba.h"

#include "helper/PerformanceMonitor.h"
#include "helper/ErrorHandler.h"

#include "audio/Sound.h"

#include "input/InputManager.h"

#include "net/NetworkManager.h"

#include "fx/Light.h"
#include "fx/ParticleManager.h"

#include "gui/gui.h"
#include "gui/Picture.h"
#include "gui/Text.h"

#include "y/EngineData.h"
#include "meta.h"


#include "plugins/PluginManager.h"
#include "plugins/Controller.h"

#include "renderer/RenderPath.h"
#include "renderer/RenderPathGL.h"
#include "renderer/RenderPathGLForward.h"
#include "renderer/RenderPathGLDeferred.h"

#include "Config.h"
#include "world/Camera.h"
#include "world/Material.h"
#include "world/Model.h"
#include "world/Object.h"
#include "world/Terrain.h"
#include "world/World.h"

bool SHOW_GBUFFER = false;
bool SHOW_SHADOW = false;

const string app_name = "y";
const string app_version = "0.1.0";



// pipeline: shader + z buffer / blending parameters

// descriptor set: textures + shader uniform buffers

void DrawSplashScreen(const string &str, float per) {
	std::cerr << " - splash - " << str.c_str() << "\n";
}

void ExternalModelCleanup(Model *m) {}



string ObjectDir;

using namespace std::chrono;


namespace nix {
void TextureClear();
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

#if HAS_LIB_VULKAN
	WindowRendererVulkan *renderer;
#endif
	RenderPath *render_path;

	PerformanceMonitor perf_mon;

	gui::Text *fps_display;

	void init(const Array<string> &arg) {
		config.load();

		window = create_window();
		kaba::init();
		NetworkManager::init();

		engine.app_name = app_name;
		engine.version = app_version;
		if (config.get_str("error.missing-files", "ignore") == "ignore")
			engine.ignore_missing_files = true;

		try {
			if (config.get_str("renderer.path", "forward") == "deferred")
				render_path = new RenderPathGLDeferred(window, engine.width, engine.height, &perf_mon);
			else
				render_path = new RenderPathGLForward(window, engine.width, engine.height, &perf_mon);
			engine.renderer = render_path;
		} catch(Exception &e) {
			msg_error(e.message());
			throw e;
		}

		std::cout << "on init..." << "\n";

		engine.set_dirs("Textures", "Maps", "Objects", "Sounds", "Scripts", "Materials", "Fonts");

		audio::init();

		GodInit();
		global_perf_mon = &perf_mon;
		PluginManager::link_kaba();

		ErrorHandler::init();



#if HAS_LIB_VULKAN
		gui::init(renderer->default_render_pass());

		if (config.get("renderer.path", "forward") == "deferred") {
			render_path = new RenderPathDeferred(renderer, &perf_mon);
		} else {
			render_path = new RenderPathForward(renderer, &perf_mon);
		}
#endif

		gui::init(render_path->shader_2d);


		InputManager::init(window);


		if (arg.num > 1)
			config.default_world = arg[1];

		MaterialInit();
		MaterialSetDefaultShader(render_path->shader_3d);
	}

	void load_first_world() {
		if (config.default_world == "")
			throw std::runtime_error("no default world defined in game.ini");

		world.reset();
		CameraReset();
		GodLoadWorld(config.default_world);
		SHOW_SHADOW = config.get_bool("shadow.debug", false);
		SHOW_GBUFFER = config.get_bool("gbuffer.debug", false);

		fps_display = new gui::Text("", 0.020f, 0.01f, 0.01f);
		fps_display->dz = 900;
		gui::toplevel->add(fps_display);
		if (SHOW_GBUFFER) {
			if (auto *rpd = dynamic_cast<RenderPathGLDeferred*>(render_path)) {
				gui::toplevel->add(new gui::Picture(rect(0.8f,1, 0.0f, 0.2f), rpd->gbuffer->color_attachments[0]));
				gui::toplevel->add(new gui::Picture(rect(0.8f,1, 0.2f, 0.4f), rpd->gbuffer->color_attachments[1]));
				gui::toplevel->add(new gui::Picture(rect(0.8f,1, 0.4f, 0.6f), rpd->gbuffer->color_attachments[2]));
				gui::toplevel->add(new gui::Picture(rect(0.8f,1, 0.6f, 0.8f), rpd->gbuffer->color_attachments[3]));
				gui::toplevel->add(new gui::Picture(rect(0.8f,1, 0.8f, 1.0f), rpd->gbuffer->depth_buffer));
			}
		}
		if (SHOW_SHADOW) {
			if (auto *rpv = dynamic_cast<RenderPathGL*>(render_path)) {
				gui::toplevel->add(new gui::Picture(rect(0, 0.2f, 0.8f, 1.0f), rpv->fb_shadow->depth_buffer));
				gui::toplevel->add(new gui::Picture(rect(0.2f, 0.4f, 0.8f, 1.0f), rpv->fb_shadow2->depth_buffer));
			}
		}

		for (auto &s: world.scripts)
			plugin_manager.add_controller(s.filename, s.variables);
		for (auto &s: config.get_str("additional-scripts", "").explode(","))
			plugin_manager.add_controller(s, {});
	}
	
	GLFWwindow* create_window() {
		GLFWwindow* window;
		glfwInit();
#if HAS_LIB_VULKAN
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#endif

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);

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
		return window;
	}

	void main_loop() {
		while (!glfwWindowShouldClose(window)) {
			InputManager::iterate();
			perf_mon.frame();
			engine.elapsed_rt = perf_mon.frame_dt;
			engine.elapsed = perf_mon.frame_dt;

			plugin_manager.handle_input();

			iterate();
			draw_frame();

			if (InputManager::get_key(hui::KEY_CONTROL) and InputManager::get_key(hui::KEY_Q))
				break;
		}

#if HAS_LIB_VULKAN
		vkDeviceWaitIdle(vulkan::device);
#endif
	}

	void reset_game() {
		EntityManager::enabled = false;
		world.reset();
		gui::reset();
		EntityManager::enabled = true;
	}

	void cleanup() {
		reset_game();
		GodEnd();

		delete render_path;
#if HAS_LIB_VULKAN
		delete renderer;
		vulkan::destroy();
#endif

		glfwDestroyWindow(window);

		glfwTerminate();
	}

	void iterate() {
		perf_mon.tick(PMLabel::UNKNOWN);
		plugin_manager.handle_iterate_pre(engine.elapsed);
		network_manager.iterate();
		world.iterate(engine.elapsed);
		for (auto *o: world.objects)
			if (o)
				o->on_iterate(engine.elapsed);
		plugin_manager.handle_iterate(engine.elapsed);
		world.particle_manager->iterate(engine.elapsed);
		gui::iterate(engine.elapsed);
		perf_mon.tick(PMLabel::ITERATE);

		world.iterate_animations(engine.elapsed);
		perf_mon.tick(PMLabel::ANIMATION);
	}







	void update_statistics() {
#if HAS_LIB_VULKAN
		vulkan::wait_device_idle();
#endif
		fps_display->set_text(format("%.1f", 1.0f / perf_mon.avg.frame_time));
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
			if (dt > target_dt * 1.08f) {
				render_path->resolution_scale_x /= 1.20f;
			} else if (dt < target_dt * 1.02f) {
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
		if (perf_mon.frames == 0)
			update_statistics();

		update_dynamic_resolution();

		render_path->start_frame();
		plugin_manager.handle_draw_pre();
		timer_render.peek();
		render_path->draw();
		render_times.add(timer_render.get());
		render_path->end_frame();
	}


};


Array<float> YEngineApp::render_times;


YEngineApp app;

#if HAS_LIB_VULKAN
vulkan::DescriptorSet *rp_create_dset(const Array<vulkan::Texture*> &tex, vulkan::UniformBuffer *ubo) {
	auto *rpv = dynamic_cast<RenderPathVulkan*>(app.render_path);
	return rpv->rp_create_dset(tex, ubo);
}

vulkan::DescriptorSet *rp_create_dset_fx(vulkan::Texture *tex, vulkan::UniformBuffer *ubo) {
	auto *rpv = dynamic_cast<RenderPathVulkan*>(app.render_path);
	return rpv->rp_create_dset_fx(tex, ubo);
}
#endif


int hui_main(const Array<string> &arg) {

	hui::Application::guess_directories(arg, "y");

	try {
		app.run(arg);
	} catch (const std::exception& e) {
		msg_error(e.what());
		return EXIT_FAILURE;
	} catch (const Exception& e) {
		msg_error(e.message());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
