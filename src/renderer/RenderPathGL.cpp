/*
 * RenderPathGL.cpp
 *
 *  Created on: 07.08.2020
 *      Author: michi
 */

#include <GLFW/glfw3.h>

#include "RenderPathGL.h"
#include "../lib/nix/nix.h"
#include "../lib/image/image.h"

#include "../helper/PerformanceMonitor.h"
#include "../plugins/PluginManager.h"
#include "../fx/Light.h"
#include "../gui/gui.h"
#include "../gui/Picture.h"
#include "../world/Camera.h"
#include "../world/Material.h"
#include "../Config.h"
#include "../meta.h"

namespace nix {
	void resolve_multisampling(FrameBuffer *target, FrameBuffer *source);
}

nix::Texture *_tex_white;

const int CUBE_SIZE = 128;

void break_point() {
	if (config.debug) {
		glFlush();
		glFinish();
	}
}

Array<Material*> post_processors;


const float HALTON2[] = {1/2.0f, 1/4.0f, 3/4.0f, 1/8.0f, 5/8.0f, 3/8.0f, 7/8.0f, 1/16.0f, 9/16.0f, 3/16.0f, 11/16.0f, 5/16.0f, 13/16.0f};
const float HALTON3[] = {1/3.0f, 2/3.0f, 1/9.0f, 4/9.0f, 7/9.0f, 2/9.0f, 5/9.0f, 8/9.0f, 1/27.0f, 10/27.0f, 19/27.0f, 2/27.0f, 11/27.0f, 20/27.0f};

static int jitter_frame = 0;

matrix jitter(float w, float h, int uid) {
	int u = (jitter_frame + uid * 2) % 13;
	int v = (jitter_frame + uid * 5) % 14;
	return matrix::translation(vector((HALTON2[u] - 0.5f) / w, (HALTON3[v] - 0.5f) / h, 0));
}

void jitter_iterate() {
	jitter_frame ++;
}

RenderPathGL::RenderPathGL(GLFWwindow* win, int w, int h, PerformanceMonitor *pm) {
	window = win;
	glfwMakeContextCurrent(window);
	//glfwGetFramebufferSize(window, &width, &height);
	width = w;
	height = h;

	perf_mon = pm;

	nix::init();

	shadow_box_size = config.get_float("shadow.boxsize", 2000);
	shadow_resolution = config.get_int("shadow.resolution", 1024);
	shadow_index = -1;

	ubo_light = new nix::UniformBuffer();
	tex_white = new nix::Texture();
	tex_black = new nix::Texture();
	Image im;
	im.create(16, 16, White);
	tex_white->overwrite(im);
	im.create(16, 16, Black);
	tex_black->overwrite(im);

	_tex_white = tex_white.get();

	vb_2d = new nix::VertexBuffer("3f,3f,2f");
	vb_2d->create_rect(rect(-1,1, -1,1));

	depth_cube = new nix::DepthBuffer(CUBE_SIZE, CUBE_SIZE);
	fb_cube = nullptr;
	cube_map = new nix::CubeMap(CUBE_SIZE);

	EntityManager::enabled = false;
	//shadow_cam = new Camera(v_0, quaternion::ID, rect::ID);
	EntityManager::enabled = true;
}

void RenderPathGL::render_into_cubemap(nix::DepthBuffer *depth, nix::CubeMap *cube, const vector &pos) {
	if (!fb_cube)
		fb_cube = new nix::FrameBuffer({depth});
	Camera cam(pos, quaternion::ID, rect::ID);
	cam.fov = pi/2;
	for (int i=0; i<6; i++) {
		try{
			fb_cube->update_x({cube, depth}, i);
		}catch(Exception &e){
			msg_error(e.message());
			return;
		}
		if (i == 0)
			cam.ang = quaternion::rotation(vector(0,-pi/2,0));
		if (i == 1)
			cam.ang = quaternion::rotation(vector(0,pi/2,0));
		if (i == 2)
			cam.ang = quaternion::rotation(vector(pi/2,0,pi));
		if (i == 3)
			cam.ang = quaternion::rotation(vector(-pi/2,0,pi));
		if (i == 4)
			cam.ang = quaternion::rotation(vector(0,pi,0));
		if (i == 5)
			cam.ang = quaternion::rotation(vector(0,0,0));
		render_into_texture(fb_cube.get(), &cam);
	}
}

nix::FrameBuffer *RenderPathGL::next_fb(nix::FrameBuffer *cur) {
	return (cur == fb4) ? fb5.get() : fb4.get();
}

nix::FrameBuffer* RenderPathGL::do_post_processing(nix::FrameBuffer *source) {
	auto cur = source;
	auto next = next_fb(cur);
	for (auto *m: post_processors) {
		nix::set_shader(m->shader.get());
		for (auto &u: m->uniforms)
			m->shader->set_data(u.location, u.p, u.size);
		process({cur->color_attachments[0], source->depth_buffer}, next, m->shader.get());
		cur = next;
		next = next_fb(cur);
	}

	if (cam->focus_enabled) {
		process_depth(cur, next, fb->depth_buffer, complex(1,0));
		cur = next;
		next = next_fb(cur);
		process_depth(cur, next, fb->depth_buffer, complex(0,1));
		cur = next;
		next = next_fb(cur);
	}

	// render blur into fb3!
	process_blur(cur, fb2.get(), 1.0f, complex(2,0));
	process_blur(fb2.get(), fb3.get(), 0.0f, complex(0,1));
	return cur;
}

nix::FrameBuffer* RenderPathGL::resolve_multisampling(nix::FrameBuffer *source) {
	auto next = fb4.get();
	if (true) {
		nix::set_shader(shader_resolve_multisample.get());
		shader_resolve_multisample->set_float(shader_resolve_multisample->get_location("width"), source->width);
		shader_resolve_multisample->set_float(shader_resolve_multisample->get_location("height"), source->height);
		process({source->color_attachments[0], source->depth_buffer}, next, shader_resolve_multisample.get());
	} else {
		// not sure, why this does not work... :(
		nix::resolve_multisampling(next, source);
	}
	return next;
}


void RenderPathGL::start_frame() {
	nix::start_frame_glfw(window);
	jitter_iterate();
}

void RenderPathGL::end_frame() {
	nix::end_frame_glfw(window);
	break_point();
	perf_mon->tick(PMLabel::END);
}


void RenderPathGL::process_blur(nix::FrameBuffer *source, nix::FrameBuffer *target, float threshold, const complex &axis) {
	nix::set_shader(shader_blur.get());
	float r = cam->bloom_radius;
	shader_blur->set_float(shader_blur->get_location("radius"), r);
	shader_blur->set_float(shader_blur->get_location("threshold"), threshold / cam->exposure);
	shader_blur->set_data(shader_blur->get_location("axis"), &axis.x, sizeof(axis));
	process(source->color_attachments, target, shader_blur.get());
}

void RenderPathGL::process_depth(nix::FrameBuffer *source, nix::FrameBuffer *target, nix::Texture *depth_buffer, const complex &axis) {
	nix::set_shader(shader_depth.get());
	shader_depth->set_float(shader_depth->get_location("max_radius"), 50);
	shader_depth->set_float(shader_depth->get_location("focal_length"), cam->focal_length);
	shader_depth->set_float(shader_depth->get_location("focal_blur"), cam->focal_blur);
	shader_depth->set_data(shader_depth->get_location("axis"), &axis.x, sizeof(axis));
	shader_depth->set_matrix(shader_depth->get_location("invproj"), cam->m_projection.inverse());
	process({source->color_attachments[0], depth_buffer}, target, shader_depth.get());
}

void RenderPathGL::process(const Array<nix::Texture*> &source, nix::FrameBuffer *target, nix::Shader *shader) {
	nix::bind_frame_buffer(target);
	nix::set_z(false, false);
	nix::set_projection_ortho_relative();
	nix::set_view_matrix(matrix::ID);
	nix::set_model_matrix(matrix::ID);
	//nix::SetShader(shader);

	nix::set_textures(source);
	nix::draw_triangles(vb_2d);
}

void RenderPathGL::draw_gui(nix::FrameBuffer *source) {
	gui::update();

	nix::set_projection_ortho_relative();
	nix::set_cull(nix::CullMode::NONE);
	nix::set_alpha(nix::Alpha::SOURCE_ALPHA, nix::Alpha::SOURCE_INV_ALPHA);
	nix::set_z(false, false);

	for (auto *n: gui::sorted_nodes) {
		if (!n->eff_visible)
			continue;
		if (n->type == n->Type::PICTURE or n->type == n->Type::TEXT) {
			auto *p = (gui::Picture*)n;
			auto shader = gui::shader.get();
			if (p->shader)
				shader = p->shader.get();
			nix::set_shader(shader);
			gui::shader->set_float(shader->get_location("blur"), p->bg_blur);
			gui::shader->set_color(shader->get_location("color"), p->eff_col);
			nix::set_textures({p->texture.get(), source->color_attachments[0]});
			if (p->angle == 0) {
				nix::set_model_matrix(matrix::translation(vector(p->eff_area.x1, p->eff_area.y1, /*0.999f - p->eff_z/1000*/ 0.5f)) * matrix::scale(p->eff_area.width(), p->eff_area.height(), 0));
			} else {
				// TODO this should use the physical ratio
				float r = (float)width / (float)height;
				nix::set_model_matrix(matrix::translation(vector(p->eff_area.x1, p->eff_area.y1, /*0.999f - p->eff_z/1000*/ 0.5f)) * matrix::scale(1/r, 1, 0) * matrix::rotation_z(p->angle) * matrix::scale(p->eff_area.width() * r, p->eff_area.height(), 0));
			}
			gui::vertex_buffer->create_rect(rect::ID, p->source);
			nix::draw_triangles(gui::vertex_buffer);
		}
	}
	nix::set_z(true, true);
	nix::set_cull(nix::CullMode::DEFAULT);

	nix::set_alpha(nix::AlphaMode::NONE);

	break_point();
	perf_mon->tick(PMLabel::GUI);
}

void RenderPathGL::render_out(nix::FrameBuffer *source, nix::Texture *bloom) {

	nix::set_textures({source->color_attachments[0], bloom});
	nix::set_shader(shader_out.get());
	shader_out->set_float(shader_out->get_location("exposure"), cam->exposure);
	shader_out->set_float(shader_out->get_location("bloom_factor"), cam->bloom_factor);
	nix::set_projection_matrix(matrix::ID);
	nix::set_view_matrix(matrix::ID);
	nix::set_model_matrix(matrix::ID);

	nix::set_z(false, false);

	nix::draw_triangles(vb_2d);

	break_point();
	perf_mon->tick(PMLabel::OUT);
}


void RenderPathGL::set_material(Material *m) {
	auto s = m->shader.get();
	nix::set_shader(s);
	s->set_data(s->get_location("eye_pos"), &cam->pos.x, 12);
	s->set_int(s->get_location("num_lights"), lights.num);
	s->set_int(s->get_location("shadow_index"), shadow_index);
	for (auto &u: m->uniforms)
		s->set_data(u.location, u.p, u.size);

	if (m->alpha.mode == TransparencyMode::FUNCTIONS)
		nix::set_alpha(m->alpha.source, m->alpha.destination);
	else if (m->alpha.mode == TransparencyMode::COLOR_KEY_HARD)
		nix::set_alpha(nix::AlphaMode::COLOR_KEY_HARD);
	else
		nix::set_alpha(nix::AlphaMode::NONE);

	set_textures(weak(m->textures));

	nix::set_material(m->albedo, m->roughness, m->metal, m->emission);
}

void RenderPathGL::set_textures(const Array<nix::Texture*> &tex) {
	auto tt = tex;
	if (tt.num == 0)
		tt.add(tex_white.get());
	if (tt.num == 1)
		tt.add(tex_white.get());
	if (tt.num == 2)
		tt.add(tex_white.get());
	tt.add(fb_shadow->depth_buffer);
	tt.add(fb_shadow2->depth_buffer);
	tt.add(cube_map.get());
	nix::set_textures(tt);
}

