

#include "lib/nix/nix.h"

#include <GLFW/glfw3.h>

#include <iostream>
#include <cstdlib>
#include <chrono>


#include "lib/math/math.h"
#include "lib/image/color.h"
#include "lib/image/image.h"
#include "lib/kaba/kaba.h"

#include "world/material.h"
#include "world/model.h"
#include "world/camera.h"
#include "world/world.h"
#include "world/object.h"
#include "world/terrain.h"

#include "helper/PerformanceMonitor.h"

#include "input/InputManager.h"

#include "fx/Light.h"
#include "fx/ParticleManager.h"

#include "gui/gui.h"
#include "gui/Picture.h"
#include "gui/Text.h"

#include "y/EngineData.h"


#include "plugins/PluginManager.h"
#include "plugins/Controller.h"

#include "renderer/RenderPath.h"
#include "renderer/RenderPathGL.h"

#include "Config.h"

const bool SHOW_GBUFFER = false;
bool SHOW_SHADOW = false;



// pipeline: shader + z buffer / blending parameters

// descriptor set: textures + shader uniform buffers

void DrawSplashScreen(const string &str, float per) {
	std::cerr << " - splash - " << str.c_str() << "\n";
}

void ExternalModelCleanup(Model *m) {}



string ObjectDir;

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

#if HAS_LIB_VULKAN
	WindowRendererVulkan *renderer;
#endif
	RenderPath *render_path;

	PerformanceMonitor perf_mon;

	gui::Text *fps_display;

	void init(const Array<string> &arg) {
		config.load();

		window = create_window();
#if HAS_LIB_VULKAN
		vulkan::init(window);
#endif
		Kaba::init();

#if HAS_LIB_VULKAN
		renderer = new WindowRendererVulkan(window);
#endif
		render_path = new RenderPathGL(window, &perf_mon);

		std::cout << "on init..." << "\n";

		engine.set_dirs("Textures", "Maps", "Objects", "Sound", "Scripts", "Materials", "Fonts");

		GodInit();
		global_perf_mon = &perf_mon;
		PluginManager::link_kaba();



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

		fps_display = new gui::Text("", 0.020f, 0.01f, 0.01f);
		fps_display->dz = 900;
		gui::toplevel->add(fps_display);
#if HAS_LIB_VULKAN
		if (SHOW_GBUFFER) {
			if (auto *rpd = dynamic_cast<RenderPathDeferred*>(render_path)) {
				gui::add(new Picture(vector(0.8f, 0.0f, 0), 0.2f, 0.2f, rpd->gbuf_ren->tex_color));
				gui::add(new Picture(vector(0.8f, 0.2f, 0), 0.2f, 0.2f, rpd->gbuf_ren->tex_emission));
				gui::add(new Picture(vector(0.8f, 0.4f, 0), 0.2f, 0.2f, rpd->gbuf_ren->tex_pos));
				gui::add(new Picture(vector(0.8f, 0.6f, 0), 0.2f, 0.2f, rpd->gbuf_ren->tex_normal));
				gui::add(new Picture(vector(0.8f, 0.8f, 0), 0.2f, 0.2f, rpd->gbuf_ren->depth_buffer));
			}
		}
#endif
		if (SHOW_SHADOW) {
			if (auto *rpv = dynamic_cast<RenderPathGL*>(render_path))
				gui::toplevel->add(new gui::Picture(rect(0, 0.2f, 0.8f, 1.0f), rpv->fb_shadow->depth_buffer));
		}

		for (auto &s: world.scripts)
			plugin_manager.add_controller(s.filename, s.variables);
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
		auto monitor = glfwGetPrimaryMonitor();
		if (!config.get_bool("screen.fullscreen", false))
			monitor = nullptr;
		engine.physical_aspect_ratio = (float)w / (float)h;

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
		}

#if HAS_LIB_VULKAN
		vkDeviceWaitIdle(vulkan::device);
#endif
	}

	void cleanup() {
		world.reset();
		GodEnd();
		gui::reset();

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
		world.iterate(engine.elapsed);
		for (auto *o: world.objects)
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




	void draw_frame() {
		if (perf_mon.frames == 0)
			update_statistics();

		plugin_manager.handle_draw_pre();
		render_path->draw();
	}


};


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
