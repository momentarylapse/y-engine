#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <cstdlib>
#include <chrono>

#include "lib/hui/hui.h"
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

#include "plugins/PluginManager.h"
#include "plugins/Controller.h"
#include "Renderer.h"

#define RENDER_TO_TEXTURE 1


// pipeline: shader + z buffer / blending parameters

// descriptor set: textures + shader uniform buffers

void DrawSplashScreen(const string &str, float per) {
	std::cerr << " - splash - " << str.c_str() << "\n";
}

void ExternalModelCleanup(Model *m) {}
extern vulkan::Shader *_default_shader_;
vulkan::Shader *shader_2d;



string ObjectDir;

using namespace std::chrono;


struct UBOMatrices {
	alignas(16) matrix model;
	alignas(16) matrix view;
	alignas(16) matrix proj;
};

struct UBOLight {
	alignas(16) vector pos;
	alignas(16) vector dir;
	alignas(16) float radius;
	float theta;
	alignas(16) color col;
};

struct UBOFog {
	alignas(16) color col;
	alignas(16) float density;
};







void cairo_render_text(const string &font_name, float font_size, const string &text, Image &im) {
	bool failed = false;
	cairo_surface_t *surface;
	cairo_t *cr;

	// initial surface size guess
	int w_surf = 512;
	int h_surf = font_size * 2;

	surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w_surf, h_surf);
	cr = cairo_create(surface);

	cairo_set_source_rgba(cr, 0, 0, 0, 1);
	cairo_rectangle(cr, 0, 0, w_surf, h_surf);
	cairo_fill(cr);

	int x = 0, y = 0;

	cairo_set_source_rgba(cr, 1, 1, 1, 1);

	PangoLayout *layout = pango_cairo_create_layout(cr);
	PangoFontDescription *desc = pango_font_description_from_string((font_name + "," + f2s(font_size, 1)).c_str());
	pango_layout_set_font_description(layout, desc);
	pango_font_description_free(desc);

	pango_layout_set_text(layout, (char*)text.data, text.num);
	//int baseline = pango_layout_get_baseline(layout) / PANGO_SCALE;
	int w_used, h_used;
	pango_layout_get_pixel_size(layout, &w_used, &h_used);

	pango_cairo_show_layout(cr, layout);
	g_object_unref(layout);

	cairo_surface_flush(surface);
	unsigned char *c0 = cairo_image_surface_get_data(surface);
	im.create(w_used, h_used, White);
	for (int y=0;y<h_used;y++) {
		unsigned char *c = c0 + 4 * y * w_surf;
		for (int x=0;x<w_used;x++) {
			float a = (float)c[1] / 255.0f;
			im.set_pixel(x, y, color(a, 1, 1, 1));
			c += 4;
		}
	}
	im.alpha_used = true;

	cairo_destroy(cr);
	cairo_surface_destroy(surface);
}

void render_text(const string &str, Image &im) {
	string font_name = "CAC Champagne";
	float font_size = 30;
	cairo_render_text(font_name, font_size, str, im);
}



class Text {
public:
	Text(const vector &p, const string &t, float h) {
		pos = p;
		text = t;
		height = h;
		col = White;
		tex = new vulkan::Texture();
		ubo = new vulkan::UBOWrapper(sizeof(UBOMatrices));
		dset = nullptr;
		rebuild();
	}
	void rebuild() {
		Image im;
		render_text(text, im);
		tex->override(&im);
		if (dset) {
			dset->set({ubo}, {tex});
		} else {
			dset = new vulkan::DescriptorSet(shader_2d->descr_layouts[0], {ubo}, {tex});
		}

		width = height * (float)im.width / (float)im.height / 1.33f;

	}
	vector pos;
	string text;
	color col;
	float height;
	float width;

	vulkan::Texture *tex;
	vulkan::UBOWrapper *ubo;
	vulkan::DescriptorSet *dset;
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

	vulkan::Pipeline *pipeline;
	vulkan::Pipeline *pipeline_2d;
	vulkan::Pipeline *pipeline_x;
	vulkan::Texture *texture_x;
	TextureRenderer *tex_ren;
	WindowRenderer *renderer;

	Text *text;
	vulkan::VertexBuffer *vertex_buffer_2d;

	void init() {
		window = create_window();
		vulkan::init(window);
		Kaba::init();

		renderer = new WindowRenderer(window);

		std::cout << "on init..." << "\n";

		engine.set_dirs("Textures/", "Maps/", "Objects/", "Sound", "Scripts/", "Materials/", "Fonts/");

		PluginManager::link_kaba();


		auto shader = vulkan::Shader::load("3d.shader");
		_default_shader_ = shader;
		pipeline = vulkan::Pipeline::build(shader, renderer->default_render_pass, 1, false);
		//pipeline->wireframe = true;
		pipeline->create();


		shader_2d = vulkan::Shader::load("2d.shader");
		pipeline_2d = vulkan::Pipeline::build(shader_2d, renderer->default_render_pass, 1, false);
		pipeline_2d->set_blend(VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
		pipeline_2d->create();

		Image im_x;
		im_x.create(512,512, Black);
		texture_x = new vulkan::Texture();
		texture_x->override(&im_x);

		tex_ren = new TextureRenderer(texture_x);

		pipeline_x = vulkan::Pipeline::build(shader, tex_ren->render_pass, 1);
		/*pipeline_x = vulkan::Pipeline::build(shader_2d, tex_ren->render_pass, 1, false);
		pipeline_x->set_blend(VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
		pipeline_x->create();*/

		vulkan::descriptor_pool = vulkan::create_descriptor_pool();
		
		game_ini.load();

		MaterialInit();

		vertex_buffer_2d = new vulkan::VertexBuffer();
		Array<vulkan::Vertex1> vertices;
		vertices.add({vector(0,0,0), vector::EZ, 0,0});
		vertices.add({vector(0,1,0), vector::EZ, 0,1});
		vertices.add({vector(1,0,0), vector::EZ, 1,0});
		vertices.add({vector(1,1,0), vector::EZ, 1,1});
		vertex_buffer_2d->build1i(vertices, {0,2,1, 1,2,3});
	}

	void load_first_world() {
		if (game_ini.default_world == "")
			throw std::runtime_error("no default world defined in game.ini");

		world.reset();
		CameraReset();
		GodLoadWorld(game_ini.default_world);

		text = new Text(vector(0.05f,0.05f,0), "Hallo, kleiner Test äöü", 0.05f);

		for (auto &s: world.scripts)
			plugin_manager.add_controller(s.filename);
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

	void render_gui(vulkan::CommandBuffer *cb) {
		cb->set_pipeline(pipeline_2d);

		UBOMatrices u;
		u.proj = (matrix::translation(vector(-1,-1,0)) * matrix::scale(2,2,1)).transpose();
		u.view = matrix::ID.transpose();
		u.model = (matrix::translation(text->pos) * matrix::scale(text->width, text->height, 1)).transpose();
		text->ubo->update(&u);

		cb->bind_descriptor_set(0, text->dset);
		cb->draw(vertex_buffer_2d);
	}

	void prepare_all() {

		cam->set_view();

		world.fog._color = Red;
		world.fog.density = 0.01f;

		UBOMatrices u;
		u.proj = cam->m_projection.transpose();
		u.view = cam->m_view.transpose();

		UBOLight l;
		l.col = Black;
		if (world.sun->enabled) {
			l.pos = world.sun->pos;
			l.dir = world.sun->dir;
			l.col = world.sun->col;
			l.radius = world.sun->radius;
			l.theta = world.sun->radius;
		}
		world.ubo_light->update(&l);

		UBOFog f;
		f.col = world.fog._color;
		f.density = world.fog.density;
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

	void render_all(vulkan::CommandBuffer *cb, vulkan::RenderPass *rp, vulkan::Pipeline *pip, vulkan::FrameBuffer *fb) {
		rp->clear_color = world.background;
		cb->begin_render_pass(rp, fb);
		cb->set_pipeline(pip);

		draw_world(cb);

		render_gui(cb);

		cb->end_render_pass();

	}

#if RENDER_TO_TEXTURE
	void render_to_texture() {
		tex_ren->start_frame();
		auto *cb = tex_ren->cb;
		cam->set_view();

		cb->begin();

		tex_ren->render_pass->clear_color = Red;
		cb->begin_render_pass(tex_ren->render_pass, tex_ren->current_frame_buffer());
		cb->set_pipeline(pipeline_x);

		for (auto &s: world.sorted_opaque) {
			Model *m = s.model;

			cb->bind_descriptor_set(0, s.dset);
			cb->draw(m->mesh[0]->sub[0].vertex_buffer);
		}
		cb->end_render_pass();
		cb->end();

		tex_ren->end_frame();
		vulkan::wait_device_idle();
	}
#endif

	void draw_frame() {
		speedometer.tick(text);


		static auto start_time = high_resolution_clock::now();

		auto current_time = high_resolution_clock::now();
		time = duration<float, seconds::period>(current_time - start_time).count();

		prepare_all();

#if RENDER_TO_TEXTURE
		render_to_texture();

		world.terrains[0]->dset->set({world.terrains[0]->ubo, world.ubo_light, world.ubo_fog}, {texture_x});
#endif

		if (!renderer->start_frame())
			return;
		auto cb = renderer->cb;
		cam->set_view();


		cb->begin();
		render_all(cb, renderer->default_render_pass, pipeline, renderer->current_frame_buffer());
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
