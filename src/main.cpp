

#include "lib/nix/nix.h"

#if HAS_LIB_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>

#include <iostream>
#include <cstdlib>
#include <chrono>


#if HAS_LIB_VULKAN
#include "lib/vulkan/vulkan.h"
#endif
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
#include "helper/InputManager.h"

#include "fx/Light.h"
#include "fx/Particle.h"

#include "gui/Picture.h"
#include "gui/Text.h"

#include "plugins/PluginManager.h"
#include "plugins/Controller.h"

#include "renderer/RenderPath.h"
#if HAS_LIB_VULKAN
#include "renderer/vulkan/WindowRendererVulkan.h"
#include "renderer/vulkan/GBufferRenderer.h"
#include "renderer/vulkan/RenderPathDeferred.h"
#include "renderer/vulkan/ShadowMapRenderer.h"

#include "renderer/vulkan/RenderPathForward.h"
#endif

const bool SHOW_GBUFFER = false;
const bool SHOW_SHADOW = false;


// pipeline: shader + z buffer / blending parameters

// descriptor set: textures + shader uniform buffers

void DrawSplashScreen(const string &str, float per) {
	std::cerr << " - splash - " << str.c_str() << "\n";
}

void ExternalModelCleanup(Model *m) {}



string ObjectDir;

using namespace std::chrono;





class GameIni {
public:
	string main_script;
	string default_world;
	string second_world;
	string default_material;
	string default_font;

	void load() {
		File *f = FileOpenText("game.ini");
		f->read_comment();
		main_script = f->read_str();
		f->read_comment();
		default_world = f->read_str();
		f->read_comment();
		second_world = f->read_str();
		f->read_comment();
		default_material = f->read_str();
		f->read_comment();
		default_font = f->read_str();
		delete f;
	}
};
static GameIni game_ini;

class Config {
public:
	Map<string,string> map;
	void load() {
		File *f = FileOpenText("config.txt");
		while(!f->eof()) {
			string s = f->read_str();
			if (s.num == 0)
				continue;
			if (s[0] == '#')
				continue;
			int p = s.find("=");
			if (p >= 0) {
				map.set(s.head(p).replace(" ", ""), s.substr(p+1, -1).replace(" ", ""));
			}
		}
		for (auto &k: map.keys())
			msg_write("config:  " + k + " == " + map[k]);
		delete f;
	}
	string get(const string &key, const string &def) {
		if (map.find(key) >= 0)
			return map[key];
		return def;
	}
	bool get_bool(const string &key, bool def) {
		string s = get(key, def ? "yes" : "no");
		return s == "yes" or s == "true" or s == "1";
	}
};
static Config config;


class RenderPathGL : public RenderPath {
public:
	int width, height;
	GLFWwindow* window;
	RenderPathGL(GLFWwindow* w) {
		window = w;
		glfwMakeContextCurrent(window);
		glfwGetFramebufferSize(window, &width, &height);

		nix::Init();
	}
	void draw() override {
		glfwGetFramebufferSize(window, &width, &height);
		nix::SetViewport(width, height);
		cam->set_view(1.3f);
		nix::SetProjectionMatrix(matrix::scale(1,-1,1) * cam->m_projection);
		nix::SetViewMatrix(cam->m_view);

		nix::ResetToColor(world.background);
		nix::ResetZ();

		nix::SetZ(true, true);

		nix::SetLightDirectional(0, world.lights[0]->dir, world.lights[0]->col, world.lights[0]->harshness);


		/*for (auto *t: world.terrains) {
			gp.model = matrix::ID;
			gp.emission = Black;
			gp.xxx[0] = 0.0f;
			cb->push_constant(0, sizeof(gp), &gp);
			cb->bind_descriptor_set_dynamic(0, t->dset, {light_index});
			cb->draw(t->vertex_buffer);
		}*/

		for (auto &s: world.sorted_opaque) {
			Model *m = s.model;
			nix::SetWorldMatrix(mtr(m->pos, m->ang));//m->_matrix);
			nix::SetMaterial(White, White, White, 20, Black);
			nix::SetTextures(s.material->textures);
			nix::SetShader(s.material->shader);
			nix::DrawTriangles(m->mesh[0]->sub[s.mat_index].vertex_buffer);

			/*gp.model = mtr(m->pos, m->ang);
			gp.emission = s.material->emission;
			gp.xxx[0] = 0.2f;
			cb->push_constant(0, sizeof(gp), &gp);

			cb->bind_descriptor_set_dynamic(0, s.dset, {light_index});
			cb->draw(m->mesh[0]->sub[0].vertex_buffer);*/
		}


		glfwSwapBuffers(window);
	}
};


class YEngineApp {
public:
	
	void run() {
		init();
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

	Text *fps_display;

	void init() {
		window = create_window();
#if HAS_LIB_VULKAN
		vulkan::init(window);
#endif
		Kaba::init();

		config.load();
#if HAS_LIB_VULKAN
		renderer = new WindowRendererVulkan(window);
#endif
		render_path = new RenderPathGL(window);

		std::cout << "on init..." << "\n";

		engine.set_dirs("Textures/", "Maps/", "Objects/", "Sound", "Scripts/", "Materials/", "Fonts/");

		GodInit();
		PluginManager::link_kaba();



#if HAS_LIB_VULKAN
		gui::init(renderer->default_render_pass());

		if (config.get("renderer.path", "forward") == "deferred") {
			render_path = new RenderPathDeferred(renderer, &perf_mon);
		} else {
			render_path = new RenderPathForward(renderer, &perf_mon);
		}
#endif
		gui::init();


		InputManager::init(window);


		game_ini.load();

		MaterialInit();
	}

	void load_first_world() {
		if (game_ini.default_world == "")
			throw std::runtime_error("no default world defined in game.ini");

		world.reset();
		CameraReset();
		GodLoadWorld(game_ini.default_world);

		fps_display = new Text("Hallo, kleiner Test äöü", vector(0.02f,0.02f,0), 0.03f);
		gui::add(fps_display);
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
		if (SHOW_SHADOW) {
			if (auto *rpv = dynamic_cast<RenderPathVulkan*>(render_path))
				gui::add(new Picture(vector(0, 0.8f, 0), 0.2f, 0.2f, rpv->shadow_renderer->depth_buffer));
		}
#endif

		for (auto &s: world.scripts)
			plugin_manager.add_controller(s.filename);
	}
	
	GLFWwindow* create_window() {
		GLFWwindow* window;
		glfwInit();
#if HAS_LIB_VULKAN
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#endif

		window = glfwCreateWindow(1024, 768, "y-engine", nullptr, nullptr);
		//window = glfwCreateWindow(1024, 768, "y-engine", glfwGetPrimaryMonitor(), nullptr);
		glfwSetWindowUserPointer(window, this);
		return window;
	}

	void main_loop() {
		while (!glfwWindowShouldClose(window)) {
			InputManager::iterate();
			perf_mon.frame();
			engine.elapsed_rt = perf_mon.frame_dt;
			engine.elapsed = perf_mon.frame_dt;

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
		for (auto *c: plugin_manager.controllers)
			c->on_iterate(engine.elapsed);
		for (auto *o: world.objects)
			o->on_iterate(engine.elapsed);
	}







	void update_statistics() {
#if HAS_LIB_VULKAN
		vulkan::wait_device_idle();
#endif
		fps_display->text = format("%.1f     s:%.2f  g:%.2f  c:%.2f  fx:%.2f  2d:%.2f",
				1.0f / perf_mon.avg.frame_time,
				perf_mon.avg.location[0]*1000,
				perf_mon.avg.location[1]*1000,
				perf_mon.avg.location[2]*1000,
				perf_mon.avg.location[3]*1000,
				perf_mon.avg.location[4]*1000);
		fps_display->rebuild();
	}




	void draw_frame() {
//		if (perf_mon.frames == 0)
//			update_statistics();

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
	msg_init();

	try {
		app.run();
	} catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
