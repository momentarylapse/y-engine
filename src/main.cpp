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


// pipeline: shader + z buffer / blending parameters

// descriptor set: textures + shader uniform buffers

void DrawSplashScreen(const string &str, float per) {
	std::cerr << " - splash - " << str.c_str() << "\n";
}

void ExternalModelCleanup(Model *m) {}
extern vulkan::Shader *_default_shader_;
vulkan::Shader *shader_2d;
vulkan::Pipeline *pipeline_2d;

string ObjectDir;

using namespace std::chrono;


struct UniformBufferObject {
	alignas(16) matrix model;
	alignas(16) matrix view;
	alignas(16) matrix proj;
};


namespace vulkan {
	extern RenderPass *render_pass;
	extern VkDescriptorPool descriptor_pool;
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
		ubo = new vulkan::UBOWrapper(sizeof(UniformBufferObject));
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

	vulkan::CommandBuffer *cb;
	vulkan::Pipeline *pipeline;


	Text *text;
	vulkan::VertexBuffer *vertex_buffer_2d;

	void init() {
		window = create_window();
		vulkan::init(window);
		Kaba::init();

		std::cout << "on init..." << "\n";

		engine.set_dirs("Textures/", "Maps/", "Objects/", "Sound", "Scripts/", "Materials/", "Fonts/");

		auto shader = vulkan::Shader::load("3d.shader");
		_default_shader_ = shader;
		pipeline = vulkan::Pipeline::build(shader, vulkan::render_pass, 1, false);
		//pipeline->wireframe = true;
		pipeline->create();


		shader_2d = vulkan::Shader::load("2d.shader");
		pipeline_2d = vulkan::Pipeline::build(shader_2d, vulkan::render_pass, 1, false);
		pipeline_2d->set_blend(VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
		pipeline_2d->create();

		vulkan::descriptor_pool = vulkan::create_descriptor_pool();

		cb = new vulkan::CommandBuffer();
		
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
	}
	
	GLFWwindow* create_window() {
		GLFWwindow* window;
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		window = glfwCreateWindow(1024, 768, "Vulkan", nullptr, nullptr);
		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, framebuffer_resize_callback);
		return window;
	}

	static void framebuffer_resize_callback(GLFWwindow* window, int width, int height) {
		vulkan::on_resize(width, height);
	}

	void main_loop() {
		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();
			draw_frame();
		}

		vkDeviceWaitIdle(vulkan::device);
	}

	void cleanup() {
		vulkan::destroy();

		glfwDestroyWindow(window);

		glfwTerminate();
	}



	matrix mtr(const vector &t, const quaternion &a) {
		auto mt = matrix::translation(t);
		auto mr = matrix::rotation_q(a);
		return mt * mr;
	}

	struct Speedometer {
		int frames = 0;
		high_resolution_clock::time_point prev = high_resolution_clock::now();
		void tick(Text *text) {
			frames ++;
		 	auto now = high_resolution_clock::now();
			float dt = std::chrono::duration<float, std::chrono::seconds::period>(now - prev).count();
			if (dt > 0.2f) {
				text->text = f2s((float)frames/dt, 1);
				text->rebuild();
				frames = 0;
				prev = now;
			}
		}
	};
	Speedometer speedometer;

	float time;

	void render_gui(vulkan::CommandBuffer *cb) {
		cb->set_pipeline(pipeline_2d);

		UniformBufferObject u;
		u.proj = (matrix::translation(vector(-1,-1,0)) * matrix::scale(2,2,1)).transpose();
		u.view = matrix::ID.transpose();
		u.model = (matrix::translation(text->pos) * matrix::scale(text->width, text->height, 1)).transpose();
		text->ubo->update(&u);

		cb->bind_descriptor_set(0, text->dset);
		cb->draw(vertex_buffer_2d);
	}

	void render_world(vulkan::CommandBuffer *cb) {
		cb->begin_render_pass(vulkan::render_pass, world.background);
		cb->set_pipeline(pipeline);

		for (auto &s: world.sorted_opaque) {
			Model *m = s.model;

			m->ang = quaternion::rotation_v(vector(-0.3f,0.5f,time));

			UniformBufferObject u;
			u.proj = cam->m_projection.transpose();
			u.view = cam->m_view.transpose();
			u.model = mtr(m->pos, m->ang).transpose();
			s.ubo->update(&u);

			cb->bind_descriptor_set(0, s.dset);
			cb->draw(m->mesh[0]->sub[0].vertex_buffer);
		}

		render_gui(cb);

		cb->end_render_pass();

	}

	void draw_frame() {
		speedometer.tick(text);

		cam->pos = 1000*vector::EZ;
		cam->set_view();

		static auto start_time = high_resolution_clock::now();

		auto current_time = high_resolution_clock::now();
		time = duration<float, seconds::period>(current_time - start_time).count();

		if (!vulkan::start_frame())
			return;

		float pc = 0;


		cb->begin();
		render_world(cb);
		cb->end();

		vulkan::submit_command_buffer(cb);

		vulkan::end_frame();

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
