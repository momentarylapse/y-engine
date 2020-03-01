

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
bool SHOW_SHADOW = false;



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
	bool debug = false;
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
		debug = get_bool("debug", false);
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
	float get_float(const string &key, float def) {
		return get(key, f2s(def, 3))._float();
	}
	int get_int(const string &key, int def) {
		return get(key, i2s(def))._int();
	}
};
static Config config;



class RenderPathGL : public RenderPath {
public:
	int width, height;
	GLFWwindow* window;
	nix::Texture *tex_black = nullptr;
	nix::Texture *tex_white = nullptr;
	nix::FrameBuffer *fb = nullptr;
	nix::FrameBuffer *fb2 = nullptr;
	nix::FrameBuffer *fb3 = nullptr;
	nix::FrameBuffer *fb4 = nullptr;
	nix::FrameBuffer *fb5 = nullptr;
	nix::FrameBuffer *fb_shadow = nullptr;
	nix::Shader *shader_blur = nullptr;
	nix::Shader *shader_depth = nullptr;
	nix::Shader *shader_out = nullptr;
	nix::Shader *shader_3d = nullptr;
	nix::Shader *shader_shadow = nullptr;

	Array<UBOLight> lights;
	nix::UniformBuffer *ubo_light;
	PerformanceMonitor *perf_mon;
	nix::VertexBuffer *vb_2d;

	//Camera *shadow_cam;
	matrix shadow_proj;

	float shadow_box_size;

	RenderPathGL(GLFWwindow* w, PerformanceMonitor *pm) {
		window = w;
		glfwMakeContextCurrent(window);
		glfwGetFramebufferSize(window, &width, &height);

		perf_mon = pm;

		shadow_box_size = config.get_float("shadow.boxsize", 2000);
		int shadow_resolution = config.get_int("shadow.resolution", 1024);
		SHOW_SHADOW = config.get_bool("shadow.debug", false);

		nix::Init();

		fb = new nix::FrameBuffer({
			new nix::Texture(width, height, "rgba:f16"),
			new nix::DepthBuffer(width, height)});
		fb2 = new nix::FrameBuffer({
			new nix::Texture(width/2, height/2, "rgba:f16")});
		fb3 = new nix::FrameBuffer({
			new nix::Texture(width/2, height/2, "rgba:f16")});
		fb4 = new nix::FrameBuffer({
			new nix::Texture(width, height, "rgba:f16")});
		fb5 = new nix::FrameBuffer({
			new nix::Texture(width, height, "rgba:f16")});
		fb_shadow = new nix::FrameBuffer({
			new nix::DepthBuffer(shadow_resolution, shadow_resolution)});

		try {
			shader_blur = nix::Shader::load("Materials/forward/blur.shader");
			shader_depth = nix::Shader::load("Materials/forward/depth.shader");
			shader_out = nix::Shader::load("Materials/forward/hdr.shader");
			shader_3d = nix::Shader::load("Materials/forward/3d.shader");
			shader_shadow = nix::Shader::load("Materials/forward/3d-shadow.shader");
		} catch(Exception &e) {
			msg_error(e.message());
			throw e;
		}
		ubo_light = new nix::UniformBuffer();
		tex_white = new nix::Texture();
		tex_black = new nix::Texture();
		Image im;
		im.create(16, 16, White);
		tex_white->overwrite(im);
		im.create(16, 16, Black);
		tex_black->overwrite(im);

		vb_2d = new nix::VertexBuffer("3f,3f,2f");
		vb_create_rect(vb_2d, rect(-1,1, -1,1));

		AllowXContainer = false;
		//shadow_cam = new Camera(v_0, quaternion::ID, rect::ID);
		AllowXContainer = true;
	}
	void draw() override {
		prepare_lights();
		perf_mon->tick(0);

		render_shadow_map();

		render_into_texture();

		int w, h;
		glfwGetFramebufferSize(window, &w, &h);
		nix::FrameBuffer::DEFAULT->width = w;
		nix::FrameBuffer::DEFAULT->height = h;


		auto *source = fb;
		if (cam->focus_enabled) {
			process_depth(source, fb4, fb->depth_buffer, true);
			process_depth(fb4, fb5, fb->depth_buffer, false);
			source = fb5;
		}
		process_blur(source, fb2, 1.0f, true);
		process_blur(fb2, fb3, 0.0f, false);


		nix::BindFrameBuffer(nix::FrameBuffer::DEFAULT);

		render_out(source, fb3);

		draw_gui();


		glfwSwapBuffers(window);
		perf_mon->tick(5);
	}

	void process_blur(nix::FrameBuffer *source, nix::FrameBuffer *target, float threshold, bool horizontal) {

		nix::SetShader(shader_blur);
		float r = cam->bloom_radius;
		complex ax;
		if (horizontal) {
			ax = complex(2,0);
		} else {
			ax = complex(0,1);
			//r /= 2;
		}
		shader_blur->set_float(shader_blur->get_location("radius"), r);
		shader_blur->set_float(shader_blur->get_location("threshold"), threshold / cam->exposure);
		shader_blur->set_data(shader_blur->get_location("axis"), &ax.x, 8);
		process(source->color_attachments, target, shader_blur);
	}

	void process_depth(nix::FrameBuffer *source, nix::FrameBuffer *target, nix::Texture *depth_buffer, bool horizontal) {

		nix::SetShader(shader_depth);
		complex ax = complex(1,0);
		if (!horizontal) {
			ax = complex(0,1);
		}
		shader_depth->set_float(shader_depth->get_location("max_radius"), 50);
		shader_depth->set_float(shader_depth->get_location("focal_length"), cam->focal_length);
		shader_depth->set_float(shader_depth->get_location("focal_blur"), cam->focal_blur);
		shader_depth->set_data(shader_depth->get_location("axis"), &ax.x, 8);
		shader_depth->set_matrix(shader_depth->get_location("invproj"), cam->m_projection.inverse());
		process({source->color_attachments[0], depth_buffer}, target, shader_depth);
	}

	void process(const Array<nix::Texture*> &source, nix::FrameBuffer *target, nix::Shader *shader) {
		nix::BindFrameBuffer(target);
		nix::SetZ(false, false);
		nix::SetProjectionOrtho(true);
		nix::SetViewMatrix(matrix::ID);
		nix::SetWorldMatrix(matrix::ID);
		//nix::SetShader(shader);

		nix::SetTextures(source);
		nix::DrawTriangles(vb_2d);
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

		if (config.debug)
			glFinish();
		perf_mon->tick(4);
	}

	void render_out(nix::FrameBuffer *source, nix::FrameBuffer *bloom) {

		nix::SetTextures({source->color_attachments[0], bloom->color_attachments[0]});
		nix::SetShader(shader_out);
		shader_out->set_float(shader_out->get_location("exposure"), cam->exposure);
		shader_out->set_float(shader_out->get_location("bloom_factor"), cam->bloom_factor);
		nix::SetProjectionMatrix(matrix::ID);
		nix::SetViewMatrix(matrix::ID);
		nix::SetWorldMatrix(matrix::ID);

		nix::SetZ(false, false);

		nix::DrawTriangles(vb_2d);

		if (config.debug)
			glFinish();
		perf_mon->tick(3);
	}

	void render_into_texture() {
		nix::BindFrameBuffer(fb);

		cam->set_view((float)width / (float)height);
		nix::SetProjectionMatrix(matrix::scale(1,-1,1) * cam->m_projection);
		nix::SetViewMatrix(cam->m_view);

		nix::ResetToColor(world.background);
		nix::ResetZ();

		nix::SetZ(true, true);

		nix::BindUniform(ubo_light, 1);


		draw_world(true);

		if (config.debug)
			glFinish();
		perf_mon->tick(2);
	}
	void draw_world(bool allow_material) {

		for (auto *t: world.terrains) {
			//nix::SetWorldMatrix(matrix::translation(t->pos));
			nix::SetWorldMatrix(matrix::ID);
			if (allow_material)
				set_material(t->material);
			t->draw();
			nix::DrawTriangles(t->vertex_buffer);
		}

		for (auto &s: world.sorted_opaque) {
			Model *m = s.model;
			nix::SetWorldMatrix(mtr(m->pos, m->ang));//m->_matrix);
			if (allow_material)
				set_material(s.material);
			nix::DrawTriangles(m->mesh[0]->sub[s.mat_index].vertex_buffer);
		}
	}
	void set_material(Material *m) {
		nix::SetShader(shader_3d);
		shader_3d->set_data(shader_3d->get_location("eye_pos"), &cam->pos.x, 16);
		shader_3d->set_int(shader_3d->get_location("num_lights"), lights.num);

		set_textures(m->textures);
		//nix::SetShader(s.material->shader);
		shader_3d->set_data(shader_3d->get_location("emission_factor"), &m->emission.r, 16);
	}
	void set_textures(const Array<nix::Texture*> &tex) {
		auto tt = tex;
		if (tt.num == 0)
			tt.add(tex_white);
		if (tt.num == 1)
			tt.add(tex_white);
		if (tt.num == 2)
			tt.add(tex_black);
		tt.add(fb_shadow->depth_buffer);
		nix::SetTextures(tt);
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

			if (l->radius <= 0){
				vector center = cam->pos + cam->ang*vector::EZ * shadow_box_size / 3;
				float grid = shadow_box_size / 16;
				center.x -= fmod(center.x, grid);
				center.y -= fmod(center.y, grid);
				center.z -= fmod(center.z, grid);
				auto t = matrix::translation(- center);
				auto r = matrix::rotation(l->dir.dir2ang()).transpose();
				float f = 1 / shadow_box_size;
				auto s = matrix::scale(f, f, f);
				// map onto [-1,1]x[-1,1]x[0,1]
				shadow_proj = matrix::translation(vector(0,0,-0.5f)) * s * r * t;
				ll.proj = shadow_proj;
			}
			lights.add(ll);
		}
		ubo_light->update(&lights[0], sizeof(UBOLight) * lights.num);
	}
	void render_shadow_map() {
		nix::BindFrameBuffer(fb_shadow);

		nix::SetProjectionMatrix(shadow_proj);
		nix::SetViewMatrix(matrix::ID);

		nix::ResetZ();

		nix::SetZ(true, true);
		nix::SetShader(shader_shadow);


		draw_world(false);

		if (config.debug)
			glFinish();
		perf_mon->tick(1);
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
#endif
		if (SHOW_SHADOW) {
			if (auto *rpv = dynamic_cast<RenderPathGL*>(render_path))
				gui::add(new Picture(vector(0, 0.8f, 0), 0.2f, 0.2f, rpv->fb_shadow->depth_buffer));
		}

		for (auto &s: world.scripts)
			plugin_manager.add_controller(s.filename);
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
		fps_display->set_text(format("%.1f\n\n shadow:\t%.2f\n world:\t%.2f\n cam:\t%.2f\n gui: \t%.2f\n xxx:\t%.2f",
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
