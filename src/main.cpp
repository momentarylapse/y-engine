

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
#include "helper/InputManager.h"

#include "fx/Light.h"
#include "fx/Particle.h"

#include "gui/Picture.h"
#include "gui/Text.h"

#include "plugins/PluginManager.h"
#include "plugins/Controller.h"

#include "renderer/RenderPath.h"

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
	nix::Texture *dyn_tex = nullptr;
	nix::DepthBuffer *depth_buffer = nullptr;
	nix::FrameBuffer *fb = nullptr;
	nix::Shader *shader_out = nullptr;
	nix::Shader *shader_3d = nullptr;

	Array<UBOLight> lights;
	nix::UniformBuffer *ubo_light;

	RenderPathGL(GLFWwindow* w) {
		window = w;
		glfwMakeContextCurrent(window);
		glfwGetFramebufferSize(window, &width, &height);

		nix::Init();

		dyn_tex = new nix::Texture(width, height, "rgba:f16");
		depth_buffer = new nix::DepthBuffer(width, height);
		fb = new nix::FrameBuffer({dyn_tex, depth_buffer});
		try {
			shader_out = nix::Shader::load("Materials/forward/hdr.shader");
			shader_3d = nix::Shader::load("Materials/forward/3d.shader");
		} catch(Exception &e) {
			msg_error(e.message());
			throw e;
		}
		ubo_light = new nix::UniformBuffer();
	}
	void draw() override {
		prepare_lights();

		render_into_texture();
		int w, h;
		glfwGetFramebufferSize(window, &w, &h);
		nix::FrameBuffer::DEFAULT->width = w;
		nix::FrameBuffer::DEFAULT->height = h;

		nix::BindFrameBuffer(nix::FrameBuffer::DEFAULT);

		render_out();

		draw_gui();


		glfwSwapBuffers(window);
	}

	void draw_gui() {
		nix::SetProjectionOrtho(true);
		nix::SetCull(CULL_NONE);
		nix::SetShader(gui::shader);
		nix::SetAlpha(ALPHA_SOURCE_ALPHA, ALPHA_SOURCE_INV_ALPHA);

		for (auto *p: gui::pictures) {
			nix::SetTexture(p->texture);
			nix::SetWorldMatrix(matrix::translation(p->pos) * matrix::scale(p->width, p->height, 0));
			nix::DrawTriangles(gui::vertex_buffer);
		}
		nix::SetCull(CULL_DEFAULT);

		nix::SetAlpha(ALPHA_NONE);
	}

	void render_out() {

		nix::SetTexture(dyn_tex);
		nix::SetShader(shader_out);
		shader_out->set_float(shader_out->get_location("exposure"), cam->exposure);
		nix::SetProjectionMatrix(matrix::ID);
		nix::SetViewMatrix(matrix::ID);
		nix::SetWorldMatrix(matrix::ID);

		nix::SetZ(false, false);



		vb_create_rect(nix::vb_temp, rect(-1,1, -1,1));

		nix::DrawTriangles(nix::vb_temp);
	}

	void render_into_texture() {
		nix::BindFrameBuffer(fb);

		cam->set_view((float)width / (float)height);
		nix::SetProjectionMatrix(matrix::scale(1,-1,1) * cam->m_projection);
		nix::SetViewMatrix(cam->m_view);

		nix::ResetToColor(world.background);
		nix::ResetZ();

		nix::SetZ(true, true);

		nix::SetLightDirectional(0, world.lights[0]->dir, world.lights[0]->col, world.lights[0]->harshness);

		nix::BindUniform(ubo_light, 1);


		for (auto *t: world.terrains) {
			//nix::SetWorldMatrix(matrix::translation(t->pos));
			nix::SetWorldMatrix(matrix::ID);
			nix::SetMaterial(White, White, White, 20, Black);
			nix::SetTextures(t->material->textures);
			//nix::SetShader(t->material->shader);
			set_shader();
			t->draw();
			nix::DrawTriangles(t->vertex_buffer);
		}

		for (auto &s: world.sorted_opaque) {
			Model *m = s.model;
			nix::SetWorldMatrix(mtr(m->pos, m->ang));//m->_matrix);
			nix::SetMaterial(White, White, White, 20, Black);
			nix::SetTextures(s.material->textures);
			//nix::SetShader(s.material->shader);
			set_shader();
			nix::DrawTriangles(m->mesh[0]->sub[s.mat_index].vertex_buffer);

			/*gp.model = mtr(m->pos, m->ang);
			gp.emission = s.material->emission;
			gp.xxx[0] = 0.2f;
			cb->push_constant(0, sizeof(gp), &gp);

			cb->bind_descriptor_set_dynamic(0, s.dset, {light_index});
			cb->draw(m->mesh[0]->sub[0].vertex_buffer);*/
		}
	}
	void set_shader() {
		nix::SetShader(shader_3d);
		shader_3d->set_data(shader_3d->get_location("eye_pos"), &cam->pos.x, 16);
		shader_3d->set_int(shader_3d->get_location("num_lights"), lights.num);
	}
	void prepare_lights() {
		lights.clear();
		for (auto *l: world.lights) {
			if (!l->enabled)
				continue;
			UBOLight ll;
			ll.pos = l->pos;
			ll.col = l->col;
			ll.dir = l->dir;
			ll.proj = l->proj;
			ll.harshness = l->harshness;
			ll.radius = l->radius;
			ll.theta = l->theta;
			lights.add(ll);
		}
		ubo_light->update(&lights[0], sizeof(UBOLight) * lights.num);
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
		fps_display->set_text(format("%.1f     s:%.2f  g:%.2f  c:%.2f  fx:%.2f  2d:%.2f",
				1.0f / perf_mon.avg.frame_time,
				perf_mon.avg.location[0]*1000,
				perf_mon.avg.location[1]*1000,
				perf_mon.avg.location[2]*1000,
				perf_mon.avg.location[3]*1000,
				perf_mon.avg.location[4]*1000));
	}




	void draw_frame() {
		if (perf_mon.frames == 0)
			update_statistics();

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
