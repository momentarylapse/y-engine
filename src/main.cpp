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

#include "fx/Light.h"

#include "gui/Picture.h"
#include "gui/Text.h"

#include "plugins/PluginManager.h"
#include "plugins/Controller.h"

#include "renderer/WindowRenderer.h"
#include "renderer/GBufferRenderer.h"
#include "renderer/TextureRenderer.h"

const bool SHOW_GBUFFER = false;
const bool SHOW_SHADOW = false;


// pipeline: shader + z buffer / blending parameters

// descriptor set: textures + shader uniform buffers

void DrawSplashScreen(const string &str, float per) {
	std::cerr << " - splash - " << str.c_str() << "\n";
}

void ExternalModelCleanup(Model *m) {}
extern vulkan::Shader *_default_shader_;



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

class ShadowMapRenderer : public Renderer {
public:
	ShadowMapRenderer() {
		width = 512;
		height = 512;

		depth_buffer = new vulkan::DepthBuffer(width, height, "d:f32", true);

		_default_render_pass = new vulkan::RenderPass({depth_buffer->format}, true, false);
		frame_buffer = new vulkan::FrameBuffer(width, height, _default_render_pass, {depth_buffer->view});


		//shader_into_gbuf = vulkan::Shader::load("3d-multi.shader");
		shader = vulkan::Shader::load("3d-shadow2.shader");
		pipeline = new vulkan::Pipeline(shader, _default_render_pass, 1);

	}
	~ShadowMapRenderer() {
		delete _default_render_pass;
		delete pipeline;
		delete shader;
		delete frame_buffer;
		delete depth_buffer;
	}

	vulkan::DepthBuffer *depth_buffer;
	vulkan::FrameBuffer *frame_buffer;

	vulkan::Shader *shader;
	vulkan::Pipeline *pipeline;

	vulkan::RenderPass *_default_render_pass;

	bool start_frame() override {
		//in_flight_fence->wait();
		cb->begin();
		return true;

	}
	void end_frame() override {
		//cb->barrier({tex_color, tex_emission, tex_normal, tex_pos, depth_buffer}, 0);
		cb->barrier({depth_buffer}, 0);
		cb->end();
		vulkan::queue_submit_command_buffer(cb, {}, {}, in_flight_fence);
		in_flight_fence->wait();
	}
	vulkan::RenderPass *default_render_pass() override { return _default_render_pass; }
	vulkan::FrameBuffer *current_frame_buffer() override {
		return frame_buffer;
	}

};

/*class ShadowMapRenderer : public TextureRenderer {
public:
	vulkan::Shader *shader;
	vulkan::Pipeline *pipeline;

	ShadowMapRenderer() : TextureRenderer(new vulkan::DynamicTexture(512, 512, 1, "rgba:i8")) {
		shader = vulkan::Shader::load("3d-shadow.shader");
		pipeline = new vulkan::Pipeline(shader, default_render_pass(), 1);
	}

	~ShadowMapRenderer() {
		delete pipeline;
		delete shader;
	}
};*/




class DeferredRenderer : public Renderer {
public:
	DeferredRenderer(WindowRenderer *output_renderer, GBufferRenderer *gbuf_ren) {
		shader_merge_base = vulkan::Shader::load("2d-gbuf-emission.shader");
		shader_merge_light = vulkan::Shader::load("2d-gbuf-light.shader");
		shader_merge_light_shadow = vulkan::Shader::load("2d-gbuf-light-shadow.shader");
		shader_merge_fog = vulkan::Shader::load("2d-gbuf-fog.shader");
		//pipeline_merge = new vulkan::Pipeline(shader_merge_base, render_pass_merge, 1);



		pipeline_x1 = new vulkan::Pipeline(shader_merge_base, output_renderer->default_render_pass(), 1);
		pipeline_x1->set_z(false, false);
		pipeline_x1->rebuild();

		pipeline_x2 = new vulkan::Pipeline(shader_merge_light, output_renderer->default_render_pass(), 1);
		pipeline_x2->set_dynamic({"scissor"});
		pipeline_x2->set_blend(VK_BLEND_FACTOR_SRC_COLOR, VK_BLEND_FACTOR_ONE);
		pipeline_x2->set_z(false, false);
		pipeline_x2->rebuild();

		pipeline_x2s = new vulkan::Pipeline(shader_merge_light_shadow, output_renderer->default_render_pass(), 1);
		pipeline_x2s->set_dynamic({"scissor"});
		pipeline_x2s->set_blend(VK_BLEND_FACTOR_SRC_COLOR, VK_BLEND_FACTOR_ONE);
		pipeline_x2s->set_z(false, false);
		pipeline_x2s->rebuild();

		pipeline_x3 = new vulkan::Pipeline(shader_merge_fog, output_renderer->default_render_pass(), 1);
		pipeline_x3->set_blend(VK_BLEND_FACTOR_SRC_COLOR, VK_BLEND_FACTOR_ONE);
		pipeline_x3->set_z(false, false);
		pipeline_x3->rebuild();



		ubo_x1 = new vulkan::UBOWrapper(sizeof(UBOMatrices));
		dset_x1 = new vulkan::DescriptorSet(shader_merge_base->descr_layouts[0], {ubo_x1, world.ubo_light, world.ubo_fog}, {gbuf_ren->tex_color, gbuf_ren->tex_emission, gbuf_ren->tex_pos, gbuf_ren->tex_normal});
	}
	~DeferredRenderer() override {
		delete dset_x1;
		delete ubo_x1;
		delete pipeline_x1;
		delete pipeline_x2;
		delete pipeline_x2s;
		delete pipeline_x3;

		delete shader_merge_base;
		delete shader_merge_light;
		delete shader_merge_light_shadow;
		delete shader_merge_fog;
	}

	/*bool start_frame_into_gbuf();
	void end_frame_into_gbuf();


	vulkan::Texture *tex_output;

	//vulkan::DepthBuffer *depth_buffer;
	vulkan::FrameBuffer *frame_buffer;*/

	vulkan::UBOWrapper *ubo_x1;
	vulkan::DescriptorSet *dset_x1;

	vulkan::Shader *shader_merge_base;
	vulkan::Shader *shader_merge_light;
	vulkan::Shader *shader_merge_light_shadow;
	vulkan::Shader *shader_merge_fog;


	vulkan::Pipeline *pipeline_x1;
	vulkan::Pipeline *pipeline_x2;
	vulkan::Pipeline *pipeline_x2s;
	vulkan::Pipeline *pipeline_x3;

	/*vulkan::Pipeline *pipeline_merge;
	vulkan::RenderPass *render_pass_merge;

	bool start_frame_merge();
	void end_frame_merge();

	vulkan::FrameBuffer *_cfb;
	vulkan::FrameBuffer *current_frame_buffer() override;*/
};

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

	vulkan::Pipeline *pipeline;
	GBufferRenderer *gbuf_ren;
	WindowRenderer *renderer;
	ShadowMapRenderer *shadow_renderer;
	DeferredRenderer *deferred_reenderer;

	Text *fps_display;
	Camera *light_cam;

	void init() {
		window = create_window();
		vulkan::init(window);
		Kaba::init();

		renderer = new WindowRenderer(window);

		std::cout << "on init..." << "\n";

		engine.set_dirs("Textures/", "Maps/", "Objects/", "Sound", "Scripts/", "Materials/", "Fonts/");

		GodInit();
		PluginManager::link_kaba();


		auto shader = vulkan::Shader::load("3d.shader");
		_default_shader_ = shader;
		pipeline = new vulkan::Pipeline(shader, renderer->default_render_pass(), 1);
		pipeline->set_dynamic({"viewport"});
		//pipeline->wireframe = true;
		pipeline->rebuild();


		gui::init(renderer->default_render_pass());

		shadow_renderer = new ShadowMapRenderer();
		gbuf_ren = new GBufferRenderer();

		deferred_reenderer = new DeferredRenderer(renderer, gbuf_ren);



		game_ini.load();

		MaterialInit();
	}

	void load_first_world() {
		if (game_ini.default_world == "")
			throw std::runtime_error("no default world defined in game.ini");

		world.reset();
		CameraReset();
		GodLoadWorld(game_ini.default_world);

		fps_display = new Text("Hallo, kleiner Test äöü", vector(0.05f,0.05f,0), 0.05f);
		gui::add(fps_display);
		if (SHOW_GBUFFER) {
			gui::add(new Picture(vector(0.8f, 0.0f, 0), 0.2f, 0.2f, gbuf_ren->tex_color));
			gui::add(new Picture(vector(0.8f, 0.2f, 0), 0.2f, 0.2f, gbuf_ren->tex_emission));
			gui::add(new Picture(vector(0.8f, 0.4f, 0), 0.2f, 0.2f, gbuf_ren->tex_pos));
			gui::add(new Picture(vector(0.8f, 0.6f, 0), 0.2f, 0.2f, gbuf_ren->tex_normal));
			gui::add(new Picture(vector(0.8f, 0.8f, 0), 0.2f, 0.2f, gbuf_ren->depth_buffer));
		}
		if (SHOW_SHADOW) {
			gui::add(new Picture(vector(0, 0.8f, 0), 0.2f, 0.2f, shadow_renderer->depth_buffer));
		}

		for (auto &s: world.scripts)
			plugin_manager.add_controller(s.filename);

		light_cam = new Camera(v_0, quaternion::ID, rect::ID);
	}
	
	GLFWwindow* create_window() {
		GLFWwindow* window;
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		window = glfwCreateWindow(1024, 768, "Vulkan", nullptr, nullptr);
		glfwSetWindowUserPointer(window, this);
		return window;
	}

	void main_loop() {
		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();
			iterate();
			draw_frame();
		}

		vkDeviceWaitIdle(vulkan::device);
	}

	void cleanup() {
		world.reset();
		GodEnd();
		gui::reset();

		delete deferred_reenderer;
		delete shadow_renderer;
		delete pipeline;
		delete gbuf_ren;
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

	struct Speedometer {
		int frames = 0;
		high_resolution_clock::time_point prev = high_resolution_clock::now();
		high_resolution_clock::time_point prev_slow = high_resolution_clock::now();
		void tick(Text *text) {
			frames ++;
		 	auto now = high_resolution_clock::now();
			float dt = std::chrono::duration<float, std::chrono::seconds::period>(now - prev).count();
			engine.elapsed = dt;
			prev = now;
			float dt_slow = std::chrono::duration<float, std::chrono::seconds::period>(now - prev_slow).count();
			if (dt_slow > 0.2f) {
				text->text = f2s((float)frames/dt_slow, 1);
				text->rebuild();
				frames = 0;
				prev_slow = now;
			}
		}
	};
	Speedometer speedometer;

	float time;


	void prepare_all(Renderer *r, Camera *c) {

		c->set_view((float)r->width / (float)r->height);

		UBOMatrices u;
		u.proj = c->m_projection.transpose();
		u.view = c->m_view.transpose();

		UBOFog f;
		f.col = world.fog._color;
		f.distance = world.fog.distance;
		world.ubo_fog->update(&f);

		for (auto *t: world.terrains) {
			u.model = matrix::ID.transpose();
			t->ubo->update(&u);

			t->draw(); // rebuild stuff...
		}
		for (auto &s: world.sorted_opaque) {
			Model *m = s.model;

			u.model = mtr(m->pos, m->ang).transpose();
			s.ubo->update(&u);
		}

		gui::update();
	}

	void draw_world(vulkan::CommandBuffer *cb) {
		for (auto *t: world.terrains) {
			cb->bind_descriptor_set(0, t->dset);
			cb->draw(t->vertex_buffer);
		}

		for (auto &s: world.sorted_opaque) {
			Model *m = s.model;

			cb->bind_descriptor_set(0, s.dset);
			cb->draw(m->mesh[0]->sub[0].vertex_buffer);
		}

	}

	void render_all(Renderer *r) {
		auto *cb = r->cb;
		auto *rp = r->default_render_pass();
		auto *fb = r->current_frame_buffer();
		cur_cam = cam;

		cb->set_viewport(r->area());

		//rp->clear_color[0] = world.background;
		cb->begin_render_pass(rp, fb);

		//draw_world(cb);

		draw_from_gbuf(cb, deferred_reenderer);

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


	vector project_pixel(const vector &v) {
		vector p = cam->project(v);
		return vector(p.x * renderer->width, (1-p.y) * renderer->height, p.z);
	}

	float projected_sphere_radius(vector &v, float r) {

		vector p = project_pixel(v);
		float rmax = 0;
		static Array<vector> dirs = {vector::EX, vector::EY, vector::EZ, vector(0.7f, 0.7f, 0), vector(0.7f, 0, 0.7f), vector(0, 0.7f, 0.7f)};
		for (auto &dir: dirs) {
			float f = (p - project_pixel(v + dir * r)).length();
			rmax = max(rmax, f);
		}
		return rmax;
	}

	rect light_rect(Light *l) {
		float w = renderer->width;
		float h = renderer->height;
		if (l->radius < 0)
			return rect(0, w, 0, h);
		vector p = project_pixel(l->pos);
		float r = projected_sphere_radius(l->pos, l->radius);
		if (l->theta < 0)
			r *= 0.17f;
		else
			r *= 0.4f;

		return rect(clampf(p.x - r, 0, w), clampf(p.x + r, 0, w), clampf(p.y - r, 0, h), clampf(p.y + r, 0, h));
	}


	void draw_from_gbuf_single(vulkan::CommandBuffer *cb, vulkan::Pipeline *pip, vulkan::DescriptorSet *dset, const rect &r = rect::EMPTY) {

		cb->set_pipeline(pip);
		if (r.area() > 0)
			cb->set_scissor(r);

		cb->set_viewport(renderer->area());
		if (pip == deferred_reenderer->pipeline_x2s) {
			matrix v = light_cam->m_all.transpose();
			cb->push_constant(0, 64, &v);
			cb->push_constant(64, 12, &cam->pos);
		} else {
			cb->push_constant(0, 12, &cam->pos);
		}

		cb->bind_descriptor_set(0, dset);
		cb->draw(Picture::vertex_buffer);
	}

	void draw_from_gbuf(vulkan::CommandBuffer *cb, DeferredRenderer *r) {


		UBOMatrices u;
		u.proj = (matrix::translation(vector(-1,-1,0)) * matrix::scale(2,2,1)).transpose();
		u.view = matrix::ID.transpose();
		u.model = matrix::ID.transpose();
		r->ubo_x1->update(&u);


		// base emission
		draw_from_gbuf_single(cb, r->pipeline_x1, r->dset_x1);


		// light passes
		UBOLight l;
		for (auto *ll: world.lights) {
			if (ll->enabled) {
				l.pos = ll->pos;
				l.dir = ll->dir;
				l.col = ll->col;
				l.radius = ll->radius;
				l.theta = ll->theta;
				l.harshness = ll->harshness;
				ll->ubo->update(&l);
				if (!ll->dset)
					ll->dset = new vulkan::DescriptorSet(r->shader_merge_light->descr_layouts[0], {r->ubo_x1, ll->ubo, world.ubo_fog}, {gbuf_ren->tex_color, gbuf_ren->tex_emission, gbuf_ren->tex_pos, gbuf_ren->tex_normal, shadow_renderer->depth_buffer});
				else
					ll->dset->set({r->ubo_x1, ll->ubo, world.ubo_fog}, {gbuf_ren->tex_color, gbuf_ren->tex_emission, gbuf_ren->tex_pos, gbuf_ren->tex_normal, shadow_renderer->depth_buffer});

				if (ll == world.lights[0])
					draw_from_gbuf_single(cb, deferred_reenderer->pipeline_x2s, ll->dset, light_rect(ll));
				else
					draw_from_gbuf_single(cb, deferred_reenderer->pipeline_x2, ll->dset, light_rect(ll));
			}
		}

		// fog
		if (world.fog.enabled)
			draw_from_gbuf_single(cb, r->pipeline_x3, r->dset_x1);
	}

	void draw_frame() {
		speedometer.tick(fps_display);


		static auto start_time = high_resolution_clock::now();

		auto current_time = high_resolution_clock::now();
		time = duration<float, seconds::period>(current_time - start_time).count();

		light_cam->pos = vector(0,1000,0);
		light_cam->ang = quaternion::rotation_v(vector(pi/2, 0, 0));
		light_cam->zoom = 2;
		light_cam->min_depth = 50;
		light_cam->max_depth = 10000;
		light_cam->set_view(1.0f);

		prepare_all(shadow_renderer, light_cam);
		render_into_shadow(shadow_renderer);

		prepare_all(gbuf_ren, cam);
		render_into_gbuffer(gbuf_ren);

		prepare_all(renderer, cam);

		if (!renderer->start_frame())
			return;
		//msg_write("render-to-win");
		auto cb = renderer->cb;


		cb->begin();
		render_all(renderer);
		cb->end();

		renderer->end_frame();

		vulkan::wait_device_idle();
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
