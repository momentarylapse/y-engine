/*
 * RenderPathGL.cpp
 *
 *  Created on: 07.08.2020
 *      Author: michi
 */

#include <GLFW/glfw3.h>

#include "RenderPathGL.h"
#ifdef USING_OPENGL
#include "RendererGL.h"
#include "../lib/nix/nix.h"
#include "../lib/image/image.h"
#include "../lib/math/vector.h"
#include "../lib/math/complex.h"
#include "../lib/math/rect.h"
#include "../lib/file/msg.h"
#include "../helper/PerformanceMonitor.h"
#include "../plugins/PluginManager.h"
#include "../fx/Particle.h"
#include "../fx/Beam.h"
#include "../fx/ParticleManager.h"
#include "../gui/gui.h"
#include "../gui/Picture.h"
#include "../world/Camera.h"
#include "../world/Material.h"
#include "../world/Model.h"
#include "../world/Object.h" // meh
#include "../world/Terrain.h"
#include "../world/World.h"
#include "../world/Light.h"
#include "../world/Entity3D.h"
#include "../world/components/Animator.h"
#include "../Config.h"
#include "../meta.h"


namespace nix {
	void resolve_multisampling(FrameBuffer *target, FrameBuffer *source);
	extern bool allow_separate_vertex_arrays;
}

nix::UniformBuffer *ubo_multi_matrix = nullptr;

nix::Texture *_tex_white;

const int CUBE_SIZE = 128;

void break_point() {
	if (config.debug) {
		glFlush();
		glFinish();
	}
}


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

RenderPathGL::RenderPathGL(RendererGL *r, RenderPathType _type) {
	type = _type;
	renderer = r;
	width = r->width;
	height = r->height;

	using_view_space = true;

	nix::allow_separate_vertex_arrays = true;
	nix::init();

	shadow_box_size = config.get_float("shadow.boxsize", 2000);
	shadow_resolution = config.get_int("shadow.resolution", 1024);
	shadow_index = -1;

	ubo_light = new nix::UniformBuffer();
	tex_white = new nix::Texture(16, 16, "rgba:i8");
	tex_black = new nix::Texture(16, 16, "rgba:i8");
	Image im;
	im.create(16, 16, White);
	tex_white->override(im);
	im.create(16, 16, Black);
	tex_black->override(im);

	_tex_white = tex_white.get();

	vb_2d = new nix::VertexBuffer("3f,3f,2f|i");
	vb_2d->create_rect(rect(-1,1, -1,1));

	depth_cube = new nix::DepthBuffer(CUBE_SIZE, CUBE_SIZE, "d24s8");
	fb_cube = nullptr;
	cube_map = new nix::CubeMap(CUBE_SIZE, "rgba:i8");

	//shadow_cam = new Camera(v_0, quaternion::ID, rect::ID);

	material_shadow = new Material;
	material_shadow->shader_path = "shadow.shader";

	ubo_multi_matrix = new nix::UniformBuffer();
}

void RenderPathGL::render_into_cubemap(DepthBuffer *depth, CubeMap *cube, const vector &pos) {
	if (!fb_cube)
		fb_cube = new nix::FrameBuffer({depth});
	Entity3D o(pos, quaternion::ID);
	Camera cam(rect::ID);
	cam.owner = &o;
	cam.fov = pi/2;
	for (int i=0; i<6; i++) {
		try {
			fb_cube->update_x({cube, depth}, i);
		} catch(Exception &e) {
			msg_error(e.message());
			return;
		}
		if (i == 0)
			o.ang = quaternion::rotation(vector(0,-pi/2,0));
		if (i == 1)
			o.ang = quaternion::rotation(vector(0,pi/2,0));
		if (i == 2)
			o.ang = quaternion::rotation(vector(pi/2,0,pi));
		if (i == 3)
			o.ang = quaternion::rotation(vector(-pi/2,0,pi));
		if (i == 4)
			o.ang = quaternion::rotation(vector(0,pi,0));
		if (i == 5)
			o.ang = quaternion::rotation(vector(0,0,0));
		prepare_lights(&cam);
		render_into_texture(fb_cube.get(), &cam, fb_cube->area());
	}
	cam.owner = nullptr;
}

rect RenderPathGL::dynamic_fb_area() const {
	return rect(0, fb_main->width * resolution_scale_x, 0, fb_main->height * resolution_scale_y);
}

FrameBuffer *RenderPathGL::next_fb(FrameBuffer *cur) {
	return (cur == fb2) ? fb3.get() : fb2.get();
}



// GTX750: 1920x1080 0.277 ms per trivial step
FrameBuffer* RenderPathGL::do_post_processing(FrameBuffer *source) {
	PerformanceMonitor::begin(ch_post);
	auto cur = source;

	// scripts
	for (auto &p: post_processors) {
		PerformanceMonitor::begin(p.channel);
		cur = (*p.func)(cur);
		break_point();
		PerformanceMonitor::end(p.channel);
	}


	if (cam->focus_enabled) {
		PerformanceMonitor::begin(ch_post_focus);
		auto next = next_fb(cur);
		process_depth(cur, next, complex(1,0));
		cur = next;
		next = next_fb(cur);
		process_depth(cur, next, complex(0,1));
		cur = next;
		break_point();
		PerformanceMonitor::end(ch_post_focus);
	}

	// render blur into fb3!
	PerformanceMonitor::begin(ch_post_blur);
	process_blur(cur, fb_small1.get(), 1.0f, complex(2,0));
	process_blur(fb_small1.get(), fb_small2.get(), 0.0f, complex(0,1));
	break_point();
	PerformanceMonitor::end(ch_post_blur);

	PerformanceMonitor::end(ch_post);
	return cur;
}

FrameBuffer* RenderPathGL::resolve_multisampling(FrameBuffer *source) {
	auto next = next_fb(source);
	if (true) {
		shader_resolve_multisample->set_float("width", source->width);
		shader_resolve_multisample->set_float("height", source->height);
		process({source->color_attachments[0].get(), depth_buffer}, next, shader_resolve_multisample.get());
	} else {
		// not sure, why this does not work... :(
		nix::resolve_multisampling(next, source);
	}
	return next;
}


void RenderPathGL::process_blur(FrameBuffer *source, FrameBuffer *target, float threshold, const complex &axis) {
	float r = cam->bloom_radius * resolution_scale_x;
	shader_blur->set_float("radius", r);
	shader_blur->set_float("threshold", threshold / cam->exposure);
	shader_blur->set_floats("axis", &axis.x, 2);
	process(weak(source->color_attachments), target, shader_blur.get());
}

void RenderPathGL::process_depth(FrameBuffer *source, FrameBuffer *target, const complex &axis) {
	shader_depth->set_float("max_radius", 50);
	shader_depth->set_float("focal_length", cam->focal_length);
	shader_depth->set_float("focal_blur", cam->focal_blur);
	shader_depth->set_floats("axis", &axis.x, 2);
	shader_depth->set_matrix("invproj", cam->m_projection.inverse());
	process({source->color_attachments[0].get(), depth_buffer}, target, shader_depth.get());
}

void RenderPathGL::process(const Array<Texture*> &source, FrameBuffer *target, Shader *shader) {
	nix::bind_frame_buffer(target);
	nix::set_scissor(rect(0, target->width*resolution_scale_x, 0, target->height*resolution_scale_y));
	nix::set_z(false, false);
	//nix::set_projection_ortho_relative();
	//nix::set_view_matrix(matrix::ID);
	//nix::set_model_matrix(matrix::ID);
	shader->set_floats("resolution_scale", &resolution_scale_x, 2);
	nix::set_shader(shader);

	nix::set_textures(source);
	nix::draw_triangles(vb_2d);
	nix::set_scissor(rect::EMPTY);
}

void RenderPathGL::draw_gui(FrameBuffer *source) {
	PerformanceMonitor::begin(ch_gui);
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
			auto shader = shader_2d;
			if (p->shader)
				shader = p->shader.get();
			nix::set_shader(shader);
			shader->set_float("blur", p->bg_blur);
			shader->set_color("color", p->eff_col);
			nix::set_textures({p->texture.get(), source->color_attachments[0].get()});
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

	nix::disable_alpha();

	break_point();
	PerformanceMonitor::end(ch_gui);
}

void RenderPathGL::render_out(FrameBuffer *source, Texture *bloom) {
	PerformanceMonitor::begin(ch_out);

	nix::set_textures({source->color_attachments[0].get(), bloom});
	nix::set_shader(shader_out.get());
	shader_out->set_float("exposure", cam->exposure);
	shader_out->set_float("bloom_factor", cam->bloom_factor);
	shader_out->set_float("scale_x", resolution_scale_x);
	shader_out->set_float("scale_y", resolution_scale_y);
	nix::set_projection_matrix(matrix::ID);
	nix::set_view_matrix(matrix::ID);
	nix::set_model_matrix(matrix::ID);

	nix::set_z(false, false);

	nix::draw_triangles(vb_2d);

	break_point();
	PerformanceMonitor::end(ch_out);
}


void RenderPathGL::set_material(Material *m, RenderPathType t, ShaderVariant v) {
	auto s = m->get_shader((int)t-1, v);
	nix::set_shader(s);
	if (using_view_space)
		s->set_floats("eye_pos", &cam->get_owner<Entity3D>()->pos.x, 3);
	else
		s->set_floats("eye_pos", &vector::ZERO.x, 3);
	s->set_int("num_lights", lights.num);
	s->set_int("shadow_index", shadow_index);
	for (auto &u: m->uniforms)
		s->set_floats(u.name, u.p, u.size/4);

	if (m->alpha.mode == TransparencyMode::FUNCTIONS)
		nix::set_alpha(m->alpha.source, m->alpha.destination);
	else if (m->alpha.mode == TransparencyMode::COLOR_KEY_HARD)
		nix::set_alpha(nix::Alpha::SOURCE_ALPHA, nix::Alpha::SOURCE_INV_ALPHA);
	else
		nix::disable_alpha();

	set_textures(weak(m->textures));

	nix::set_material(m->albedo, m->roughness, m->metal, m->emission);
}

void RenderPathGL::set_textures(const Array<Texture*> &tex) {
	auto tt = tex;
	if (tt.num == 0)
		tt.add(tex_white.get());
	if (tt.num == 1)
		tt.add(tex_white.get());
	if (tt.num == 2)
		tt.add(tex_white.get());
	tt.add(fb_shadow->depth_buffer.get());
	tt.add(fb_shadow2->depth_buffer.get());
	tt.add(cube_map.get());
	nix::set_textures(tt);
}




void RenderPathGL::draw_particles() {
	PerformanceMonitor::begin(ch_fx);
	nix::set_shader(shader_fx.get());
	nix::set_alpha(nix::Alpha::SOURCE_ALPHA, nix::Alpha::SOURCE_INV_ALPHA);
	nix::set_z(false, true);

	// particles
	auto r = matrix::rotation_q(cam->get_owner<Entity3D>()->ang);
	nix::vb_temp->create_rect(rect(-1,1, -1,1));
	for (auto g: world.particle_manager->groups) {
		nix::set_texture(g->texture);
		for (auto p: g->particles)
			if (p->enabled) {
				shader_fx->set_color("color", p->col);
				shader_fx->set_floats("source", &p->source.x1, 4);
				nix::set_model_matrix(matrix::translation(p->pos) * r * matrix::scale(p->radius, p->radius, p->radius));
				nix::draw_triangles(nix::vb_temp);
			}
	}

	// beams
	Array<vector> v;
	v.resize(6);
	nix::set_model_matrix(matrix::ID);
	for (auto g: world.particle_manager->groups) {
		nix::set_texture(g->texture);
		for (auto p: g->beams) {
			// TODO geometry shader!
			auto pa = cam->project(p->pos);
			auto pb = cam->project(p->pos + p->length);
			auto pe = vector::cross(pb - pa, vector::EZ).normalized();
			auto uae = cam->unproject(pa + pe * 0.1f);
			auto ube = cam->unproject(pb + pe * 0.1f);
			auto _e1 = (p->pos - uae).normalized() * p->radius;
			auto _e2 = (p->pos + p->length - ube).normalized() * p->radius;
			//vector e1 = -vector::cross(cam->ang * vector::EZ, p->length).normalized() * p->radius/2;
			v[0] = p->pos - _e1;
			v[1] = p->pos - _e2 + p->length;
			v[2] = p->pos + _e2 + p->length;
			v[3] = p->pos - _e1;
			v[4] = p->pos + _e2 + p->length;
			v[5] = p->pos + _e1;
			nix::vb_temp->update(0, v);
			shader_fx->set_color("color", p->col);
			shader_fx->set_floats("source", &p->source.x1, 4);
			nix::draw_triangles(nix::vb_temp);
		}
	}

	// script injectors
	for (auto &i: fx_injectors)
		(*i.func)();


	nix::set_z(true, true);
	nix::disable_alpha();
	break_point();
	PerformanceMonitor::end(ch_fx);
}

void RenderPathGL::draw_skyboxes(Camera *cam) {
	nix::set_z(false, false);
	nix::set_cull(nix::CullMode::NONE);
	nix::set_view_matrix(matrix::rotation_q(cam->get_owner<Entity3D>()->ang).transpose());
	for (auto *sb: world.skybox) {
		sb->_matrix = matrix::rotation_q(sb->get_owner<Entity3D>()->ang);
		nix::set_model_matrix(sb->_matrix * matrix::scale(10,10,10));
		for (int i=0; i<sb->material.num; i++) {
			set_material(sb->material[i], type, ShaderVariant::DEFAULT);
			nix::draw_triangles(sb->mesh[0]->sub[i].vertex_buffer);
		}
	}
	nix::set_cull(nix::CullMode::DEFAULT);
	nix::disable_alpha();
	break_point();
}
void RenderPathGL::draw_terrains(bool allow_material) {
	for (auto *t: world.terrains) {
		auto o = t->get_owner<Entity3D>();
		nix::set_model_matrix(matrix::translation(o->pos));
		if (allow_material) {
			set_material(t->material, type, ShaderVariant::DEFAULT);
			auto s = t->material->get_shader((int)type-1, ShaderVariant::DEFAULT);
			s->set_floats("pattern0", &t->texture_scale[0].x, 3);
			s->set_floats("pattern1", &t->texture_scale[1].x, 3);
		}
		t->prepare_draw(cam->get_owner<Entity3D>()->pos);
		nix::draw_triangles(t->vertex_buffer);
	}
}

void RenderPathGL::draw_objects_instanced(bool allow_material) {
	for (auto &s: world.sorted_multi) {
		if (!s.material->cast_shadow and !allow_material)
			continue;
		Model *m = s.model;
		nix::set_model_matrix(s.matrices[0]);//m->_matrix);
		if (allow_material)
			set_material(s.material, type, ShaderVariant::INSTANCED);
		else
			set_material(material_shadow, type, ShaderVariant::INSTANCED);
		nix::bind_buffer(ubo_multi_matrix, 5);
		//msg_write(s.matrices.num);
		nix::draw_instanced_triangles(m->mesh[0]->sub[s.mat_index].vertex_buffer, s.matrices.num);
		//s.material->shader = ss;
	}
}

void RenderPathGL::draw_objects_opaque(bool allow_material) {
	for (auto &s: world.sorted_opaque) {
		if (!s.material->cast_shadow and !allow_material)
			continue;
		Model *m = s.model;
		m->update_matrix();
		nix::set_model_matrix(m->_matrix);

		auto ani = m->owner ? m->owner->get_component<Animator>() : nullptr;

		if (ani) {
			if (allow_material)
				set_material(s.material, type, ShaderVariant::ANIMATED);
			else
				set_material(material_shadow, type, ShaderVariant::ANIMATED);
			ani->buf->update_array(ani->dmatrix);
			nix::bind_buffer(ani->buf, 7);
		} else {
			if (allow_material)
				set_material(s.material, type, ShaderVariant::DEFAULT);
			else
				set_material(material_shadow, type, ShaderVariant::DEFAULT);
		}
		nix::draw_triangles(m->mesh[0]->sub[s.mat_index].vertex_buffer);
	}
}

void RenderPathGL::draw_objects_transparent(bool allow_material, RenderPathType t) {
	nix::set_z(false, true);
	if (allow_material)
	for (auto &s: world.sorted_trans) {
		Model *m = s.model;
		m->update_matrix();
		nix::set_model_matrix(m->_matrix);
		nix::set_cull(nix::CullMode::NONE);
		auto ani = m->owner ? m->owner->get_component<Animator>() : nullptr;
		if (ani) {
			set_material(s.material, t, ShaderVariant::ANIMATED);
		} else {
			set_material(s.material, t, ShaderVariant::DEFAULT);
		}
		nix::draw_triangles(m->mesh[0]->sub[s.mat_index].vertex_buffer);
		nix::set_cull(nix::CullMode::DEFAULT);
	}
	nix::disable_alpha();
	nix::set_z(true, true);
}


void RenderPathGL::prepare_instanced_matrices() {
	PerformanceMonitor::begin(ch_pre);
	for (auto &s: world.sorted_multi) {
		ubo_multi_matrix->update_array(s.matrices);
	}
	PerformanceMonitor::end(ch_pre);
}

void RenderPathGL::prepare_lights(Camera *cam) {
	PerformanceMonitor::begin(ch_prepare_lights);

	lights.clear();
	for (auto *l: world.lights) {
		if (!l->enabled)
			continue;

		l->update(cam, shadow_box_size, using_view_space);

		if (l->allow_shadow) {
			shadow_index = lights.num;
			shadow_proj = l->shadow_projection;
		}
		lights.add(l->light);
	}
	ubo_light->update_array(lights);
	PerformanceMonitor::end(ch_prepare_lights);
}

#endif
