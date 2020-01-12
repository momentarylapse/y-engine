#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <cstdlib>
#include <chrono>

#include "lib/vulkan/vulkan.h"
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

#include "renderer/WindowRenderer.h"
#include "renderer/GBufferRenderer.h"
#include "renderer/TextureRenderer.h"
#include "renderer/DeferredRenderer.h"
#include "renderer/ShadowMapRenderer.h"

const bool SHOW_GBUFFER = false;
const bool SHOW_SHADOW = false;

extern vulkan::Shader *shader_fx;


// pipeline: shader + z buffer / blending parameters

// descriptor set: textures + shader uniform buffers

void DrawSplashScreen(const string &str, float per) {
	std::cerr << " - splash - " << str.c_str() << "\n";
}

void ExternalModelCleanup(Model *m) {}



string ObjectDir;

using namespace std::chrono;




struct UBOMatrices {
	alignas(16) matrix model;
	alignas(16) matrix view;
	alignas(16) matrix proj;
};

struct UBOFog {
	alignas(16) color col;
	alignas(16) float distance;
};




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



class YEngineApp {
public:
	
	void run() {
		init();
		load_first_world();
		main_loop();
		cleanup();
	}

private:
	GLFWwindow* window;

	WindowRenderer *renderer;
	DeferredRenderer *deferred_reenderer;

	PerformanceMonitor perf_mon;

	vulkan::Pipeline *pipeline_fx;
	vulkan::VertexBuffer *particle_vb;

	Text *fps_display;

	void init() {
		window = create_window();
		vulkan::init(window);
		Kaba::init();

		renderer = new WindowRenderer(window);

		std::cout << "on init..." << "\n";

		engine.set_dirs("Textures/", "Maps/", "Objects/", "Sound", "Scripts/", "Materials/", "Fonts/");

		GodInit();
		PluginManager::link_kaba();



		gui::init(renderer->default_render_pass());

		deferred_reenderer = new DeferredRenderer(renderer);

		shader_fx = vulkan::Shader::load("fx.shader");
		pipeline_fx = new vulkan::Pipeline(shader_fx, renderer->default_render_pass(), 0, 1);
		pipeline_fx->set_blend(VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
		pipeline_fx->set_z(true, false);
		pipeline_fx->set_culling(0);
		pipeline_fx->rebuild();

		particle_vb = new vulkan::VertexBuffer();
		Array<vulkan::Vertex1> vertices;
		vertices.add({vector(-1,-1,0), vector::EZ, 0,0});
		vertices.add({vector(-1, 1,0), vector::EZ, 0,1});
		vertices.add({vector( 1,-1,0), vector::EZ, 1,0});
		vertices.add({vector( 1, 1,0), vector::EZ, 1,1});
		particle_vb->build1i(vertices, {0,2,1, 1,2,3});


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
		if (SHOW_GBUFFER) {
			gui::add(new Picture(vector(0.8f, 0.0f, 0), 0.2f, 0.2f, deferred_reenderer->gbuf_ren->tex_color));
			gui::add(new Picture(vector(0.8f, 0.2f, 0), 0.2f, 0.2f, deferred_reenderer->gbuf_ren->tex_emission));
			gui::add(new Picture(vector(0.8f, 0.4f, 0), 0.2f, 0.2f, deferred_reenderer->gbuf_ren->tex_pos));
			gui::add(new Picture(vector(0.8f, 0.6f, 0), 0.2f, 0.2f, deferred_reenderer->gbuf_ren->tex_normal));
			gui::add(new Picture(vector(0.8f, 0.8f, 0), 0.2f, 0.2f, deferred_reenderer->gbuf_ren->depth_buffer));
		}
		if (SHOW_SHADOW) {
			gui::add(new Picture(vector(0, 0.8f, 0), 0.2f, 0.2f, deferred_reenderer->shadow_renderer->depth_buffer));
		}

		for (auto &s: world.scripts)
			plugin_manager.add_controller(s.filename);
	}
	
	GLFWwindow* create_window() {
		GLFWwindow* window;
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

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

		vkDeviceWaitIdle(vulkan::device);
	}

	void cleanup() {
		world.reset();
		GodEnd();
		gui::reset();

		delete particle_vb;
		delete pipeline_fx;
		delete deferred_reenderer;
		delete renderer;
		vulkan::destroy();

		glfwDestroyWindow(window);

		glfwTerminate();
	}

	void iterate() {
		for (auto *c: plugin_manager.controllers)
			c->on_iterate(engine.elapsed);
		for (auto *o: world.objects)
			o->on_iterate(engine.elapsed);
	}



	matrix mtr(const vector &t, const quaternion &a) {
		auto mt = matrix::translation(t);
		auto mr = matrix::rotation_q(a);
		return mt * mr;
	}


	void prepare_all(Renderer *r, Camera *c) {

		c->set_view((float)r->width / (float)r->height);

		UBOMatrices u;
		u.proj = c->m_projection;
		u.view = c->m_view;

		UBOFog f;
		f.col = world.fog._color;
		f.distance = world.fog.distance;
		world.ubo_fog->update(&f);

		for (auto *t: world.terrains) {
			u.model = matrix::ID;
			t->ubo->update(&u);

			t->draw(); // rebuild stuff...
		}
		for (auto &s: world.sorted_opaque) {
			Model *m = s.model;

			u.model = mtr(m->pos, m->ang);
			s.ubo->update(&u);
		}

		gui::update();
	}

	struct GeoPush {
		matrix model;
		color emission;
	};

	void draw_world(vulkan::CommandBuffer *cb) {

		GeoPush gp;

		for (auto *t: world.terrains) {
			gp.model = matrix::ID;
			gp.emission = Black;
			cb->push_constant(0, sizeof(gp), &gp);
			cb->bind_descriptor_set(0, t->dset);
			cb->draw(t->vertex_buffer);
		}

		for (auto &s: world.sorted_opaque) {
			Model *m = s.model;
			gp.model = mtr(m->pos, m->ang);
			gp.emission = s.material->emission;
			cb->push_constant(0, sizeof(gp), &gp);

			cb->bind_descriptor_set(0, s.dset);
			cb->draw(m->mesh[0]->sub[0].vertex_buffer);
		}

	}

	void render_fx(vulkan::CommandBuffer *cb, Renderer *r) {
		cb->set_pipeline(pipeline_fx);
		cb->set_viewport(r->area());

		cb->push_constant(80, sizeof(color), &world.fog._color);
		float density0 = 0;
		cb->push_constant(96, 4, &density0);

		for (auto *g: world.particle_manager->groups) {
			g->dset->set({}, {deferred_reenderer->gbuf_ren->depth_buffer, g->texture});
			cb->bind_descriptor_set(0, g->dset);

			for (auto *p: g->particles) {
				matrix m = cam->m_all * matrix::translation(p->pos) * matrix::rotation_q(cam->ang) * matrix::scale(p->radius, p->radius, 1);
				cb->push_constant(0, sizeof(m), &m);
				cb->push_constant(64, sizeof(color), &p->col);
				if (world.fog.enabled) {
					float dist = (cam->pos - p->pos).length(); //(cam->m_view * p->pos).z;
					float fog_density = 1-exp(-dist / world.fog.distance);
					cb->push_constant(96, 4, &fog_density);
				}
				cb->draw(particle_vb);
			}
		}

	}

	void render_all(Renderer *r) {
		auto *cb = r->cb;
		auto *rp = r->default_render_pass();
		auto *fb = r->current_frame_buffer();
		cur_cam = cam;

		cb->set_viewport(r->area());

		cb->begin_render_pass(rp, fb);

		deferred_reenderer->render_out(cb, r);
		perf_mon.tick(3);
		render_fx(cb, r);
		perf_mon.tick(4);

		gui::render(cb, r->area());

		cb->end_render_pass();

	}

	void render_into_gbuffer(GBufferRenderer *r) {
		r->start_frame();
		auto *cb = r->cb;
		cam->set_view(1.0f);

		r->default_render_pass()->clear_color[1] = world.background; // emission
		cb->begin_render_pass(r->default_render_pass(), r->current_frame_buffer());
		cb->set_pipeline(r->pipeline_into_gbuf);
		cb->set_viewport(r->area());

		draw_world(cb);
		cb->end_render_pass();

		r->end_frame();
	}

	void render_into_shadow(ShadowMapRenderer *r) {
		r->start_frame();
		auto *cb = r->cb;

		cb->begin_render_pass(r->default_render_pass(), r->current_frame_buffer());
		cb->set_pipeline(r->pipeline);
		cb->set_viewport(r->area());

		draw_world(cb);
		cb->end_render_pass();

		r->end_frame();
	}


	void update_statistics() {
		vulkan::wait_device_idle();
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
		if (perf_mon.frames == 0)
			update_statistics();

		vulkan::wait_device_idle();
		//deferred_reenderer->pick_shadow_source();

		deferred_reenderer->resize(renderer->width, renderer->height);

		if (!renderer->start_frame())
			return;

		perf_mon.tick(0);
		deferred_reenderer->light_cam->pos = vector(0,1000,0);
		deferred_reenderer->light_cam->ang = quaternion::rotation_v(vector(pi/2, 0, 0));
		deferred_reenderer->light_cam->zoom = 2;
		deferred_reenderer->light_cam->min_depth = 50;
		deferred_reenderer->light_cam->max_depth = 10000;
		deferred_reenderer->light_cam->set_view(1.0f);

		prepare_all(deferred_reenderer->shadow_renderer, deferred_reenderer->light_cam);
		render_into_shadow(deferred_reenderer->shadow_renderer);
		perf_mon.tick(1);

		prepare_all(deferred_reenderer->gbuf_ren, cam);
		render_into_gbuffer(deferred_reenderer->gbuf_ren);
		perf_mon.tick(2);

		prepare_all(renderer, cam);

		auto cb = renderer->cb;


		cb->begin();
		render_all(renderer);
		cb->end();

		renderer->end_frame();
		perf_mon.tick(5);
	}

};


int hui_main(const Array<string> &arg) {
	YEngineApp app;
	msg_init();

	try {
		app.run();
	} catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
