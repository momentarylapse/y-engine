/*
 * WorldRendererGL.cpp
 *
 *  Created on: 07.08.2020
 *      Author: michi
 */

#include "WorldRendererGL.h"

#include <GLFW/glfw3.h>

#ifdef USING_OPENGL
#include "../base.h"
#include "../../graphics-impl.h"
#include "../../lib/image/image.h"
#include "../../lib/math/vector.h"
#include "../../lib/math/complex.h"
#include "../../lib/math/rect.h"
#include "../../lib/file/msg.h"
#include "../../helper/PerformanceMonitor.h"
#include "../../plugins/PluginManager.h"
#include "../../fx/Particle.h"
#include "../../fx/Beam.h"
#include "../../fx/ParticleManager.h"
#include "../../gui/gui.h"
#include "../../gui/Picture.h"
#include "../../world/Camera.h"
#include "../../world/Material.h"
#include "../../world/Model.h"
#include "../../world/Object.h" // meh
#include "../../world/Terrain.h"
#include "../../world/World.h"
#include "../../world/Light.h"
#include "../../world/Entity3D.h"
#include "../../world/components/Animator.h"
#include "../../meta.h"


namespace nix {
	void resolve_multisampling(FrameBuffer *target, FrameBuffer *source);
}
void apply_shader_data(Shader *s, const Any &shader_data);

nix::UniformBuffer *ubo_multi_matrix = nullptr;

const int CUBE_SIZE = 128;



WorldRendererGL::WorldRendererGL(const string &name, Renderer *parent, RenderPathType _type) : WorldRenderer(name, parent) {
	type = _type;

	ubo_light = new nix::UniformBuffer();

	depth_cube = new nix::DepthBuffer(CUBE_SIZE, CUBE_SIZE, "d24s8");
	fb_cube = nullptr;
	cube_map = new nix::CubeMap(CUBE_SIZE, "rgba:i8");

	//shadow_cam = new Camera(v_0, quaternion::ID, rect::ID);

	vb_fx = new nix::VertexBuffer("3f,4f,2f");

	material_shadow = new Material;
	material_shadow->shader_path = "shadow.shader";

	ubo_multi_matrix = new nix::UniformBuffer();
}

void WorldRendererGL::render_into_cubemap(DepthBuffer *depth, CubeMap *cube, const vector &pos) {
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
			o.ang = quaternion::rotation(vector(0,pi/2,0));
		if (i == 1)
			o.ang = quaternion::rotation(vector(0,-pi/2,0));
		if (i == 2)
			o.ang = quaternion::rotation(vector(-pi/2,pi,pi));
		if (i == 3)
			o.ang = quaternion::rotation(vector(pi/2,pi,pi));
		if (i == 4)
			o.ang = quaternion::rotation(vector(0,0,0));
		if (i == 5)
			o.ang = quaternion::rotation(vector(0,pi,0));
		prepare_lights(&cam);
		render_into_texture(fb_cube.get(), &cam);
	}
	cam.owner = nullptr;
}



void WorldRendererGL::set_material(Material *m, RenderPathType t, ShaderVariant v) {
	auto s = m->get_shader(t, v);
	nix::set_shader(s);
	if (using_view_space)
		s->set_floats("eye_pos", &cam_main->get_owner<Entity3D>()->pos.x, 3); // NAH....
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

void WorldRendererGL::set_textures(const Array<Texture*> &tex) {
	auto tt = tex;
	if (tt.num == 0)
		tt.add(tex_white);
	if (tt.num == 1)
		tt.add(tex_white);
	if (tt.num == 2)
		tt.add(tex_white);
	tt.add(fb_shadow1->depth_buffer.get());
	tt.add(fb_shadow2->depth_buffer.get());
	tt.add(cube_map.get());
	nix::set_textures(tt);
}

void create_color_quad(VertexBuffer *vb, const rect &d, const rect &s, const color &c) {
	Array<VertexFx> v = {{{d.x1,d.y1,0}, c, s.x1,s.y1},
	                    {{d.x1,d.y2,0}, c, s.x1,s.y2},
	                    {{d.x2,d.y2,0}, c, s.x2,s.y2},
	                    {{d.x1,d.y1,0}, c, s.x1,s.y1},
	                    {{d.x2,d.y2,0}, c, s.x2,s.y2},
	                    {{d.x2,d.y1,0}, c, s.x2,s.y1}};
	vb->update(v);
}


void WorldRendererGL::draw_particles(Camera *cam) {
	PerformanceMonitor::begin(ch_fx);

	// script injectors
	for (auto &i: fx_injectors)
		if (!i.transparent)
			(*i.func)();

	nix::set_shader(shader_fx.get());
	nix::set_alpha(nix::Alpha::SOURCE_ALPHA, nix::Alpha::SOURCE_INV_ALPHA);
	nix::set_z(false, true);

	// particles
	auto r = matrix::rotation_q(cam->get_owner<Entity3D>()->ang);
	for (auto g: world.particle_manager->groups) {
		nix::set_texture(g->texture);

		Array<VertexFx> v;
		for (auto p: g->particles)
			if (p->enabled) {
				auto m = matrix::translation(p->pos) * r * matrix::scale(p->radius, p->radius, p->radius);

				v.add({m * vector(-1, 1,0), p->col, p->source.x1, p->source.y1});
				v.add({m * vector( 1, 1,0), p->col, p->source.x2, p->source.y1});
				v.add({m * vector( 1,-1,0), p->col, p->source.x2, p->source.y2});
				v.add({m * vector(-1, 1,0), p->col, p->source.x1, p->source.y1});
				v.add({m * vector( 1,-1,0), p->col, p->source.x2, p->source.y2});
				v.add({m * vector(-1,-1,0), p->col, p->source.x1, p->source.y2});
			}
		vb_fx->update(v);
		nix::set_model_matrix(matrix::ID);
		nix::draw_triangles(vb_fx);
	}

	// beams
	Array<Vertex1> v = {{v_0, v_0, 0,0}, {v_0, v_0, 0,1}, {v_0, v_0, 1,1}, {v_0, v_0, 0,0}, {v_0, v_0, 1,1}, {v_0, v_0, 1,0}};
	nix::set_model_matrix(matrix::ID);
	for (auto g: world.particle_manager->groups) {
		nix::set_texture(g->texture);

		Array<VertexFx> v;

		for (auto p: g->beams) {
			if (!p->enabled)
				continue;
			// TODO geometry shader!
			auto pa = cam->project(p->pos);
			auto pb = cam->project(p->pos + p->length);
			auto pe = vector::cross(pb - pa, vector::EZ).normalized();
			auto uae = cam->unproject(pa + pe * 0.1f);
			auto ube = cam->unproject(pb + pe * 0.1f);
			auto _e1 = (p->pos - uae).normalized() * p->radius;
			auto _e2 = (p->pos + p->length - ube).normalized() * p->radius;
			//vector e1 = -vector::cross(cam->ang * vector::EZ, p->length).normalized() * p->radius/2;

			vector p00 = p->pos - _e1;
			vector p01 = p->pos - _e2 + p->length;
			vector p10 = p->pos + _e1;
			vector p11 = p->pos + _e2 + p->length;

			v.add({p00, p->col, p->source.x1, p->source.y1});
			v.add({p01, p->col, p->source.x2, p->source.y1});
			v.add({p11, p->col, p->source.x2, p->source.y2});
			v.add({p00, p->col, p->source.x1, p->source.y1});
			v.add({p11, p->col, p->source.x2, p->source.y2});
			v.add({p10, p->col, p->source.x1, p->source.y2});
		}

		vb_fx->update(v);
		nix::draw_triangles(vb_fx);
	}

	// script injectors
	for (auto &i: fx_injectors)
		if (i.transparent)
			(*i.func)();


	nix::set_z(true, true);
	nix::disable_alpha();
	break_point();
	PerformanceMonitor::end(ch_fx);
}

void WorldRendererGL::draw_skyboxes(Camera *cam) {
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
void WorldRendererGL::draw_terrains(bool allow_material) {
	for (auto *t: world.terrains) {
		auto o = t->get_owner<Entity3D>();
		nix::set_model_matrix(matrix::translation(o->pos));
		if (allow_material) {
			set_material(t->material, type, ShaderVariant::DEFAULT);
			auto s = t->material->get_shader(type, ShaderVariant::DEFAULT);
			s->set_floats("pattern0", &t->texture_scale[0].x, 3);
			s->set_floats("pattern1", &t->texture_scale[1].x, 3);
		} else {
			set_material(material_shadow, type, ShaderVariant::DEFAULT);
		}
		t->prepare_draw(cam_main->get_owner<Entity3D>()->pos);
		nix::draw_triangles(t->vertex_buffer);
	}
}

void WorldRendererGL::draw_objects_instanced(bool allow_material) {
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

void WorldRendererGL::draw_objects_opaque(bool allow_material) {
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

void WorldRendererGL::draw_objects_transparent(bool allow_material, RenderPathType t) {
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


void WorldRendererGL::prepare_instanced_matrices() {
	PerformanceMonitor::begin(ch_pre);
	for (auto &s: world.sorted_multi) {
		ubo_multi_matrix->update_array(s.matrices);
	}
	PerformanceMonitor::end(ch_pre);
}

void WorldRendererGL::prepare_lights(Camera *cam) {
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

void WorldRendererGL::draw_user_mesh(VertexBuffer *vb, Shader *s, const matrix &m, const Array<Texture*> &tex, const Any &data) {
	nix::set_textures(tex);
	nix::set_model_matrix(m);
	apply_shader_data(s, data);
	nix::set_shader(s);
	nix::draw_triangles(vb);
}

#endif
