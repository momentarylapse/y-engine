/*
 * GeometryRendererGL.cpp
 *
 *  Created on: Dec 15, 2022
 *      Author: michi
 */

#include "GeometryRendererGL.h"

#include <GLFW/glfw3.h>
#ifdef USING_OPENGL
#include "../WorldRendererGL.h"
#include "../../base.h"
#include "../../../lib/nix/nix.h"
#include "../../../helper/PerformanceMonitor.h"
#include "../../../world/Material.h"
#include "../../../Config.h"
#include "../../../lib/image/image.h"
#include "../../../lib/math/vec3.h"
#include "../../../lib/math/complex.h"
#include "../../../lib/math/rect.h"
#include "../../../lib/os/msg.h"
#include "../../../helper/PerformanceMonitor.h"
#include "../../../helper/ResourceManager.h"
#include "../../../plugins/PluginManager.h"
#include "../../../fx/Particle.h"
#include "../../../fx/Beam.h"
#include "../../../fx/ParticleManager.h"
#include "../../../fx/ParticleEmitter.h"
#include "../../../gui/gui.h"
#include "../../../gui/Picture.h"
#include "../../../world/Camera.h"
#include "../../../world/Material.h"
#include "../../../world/Model.h"
#include "../../../world/Object.h" // meh
#include "../../../world/Terrain.h"
#include "../../../world/World.h"
#include "../../../world/Light.h"
#include "../../../world/components/Animator.h"
#include "../../../world/components/UserMesh.h"
#include "../../../world/components/MultiInstance.h"
#include "../../../y/Entity.h"
#include "../../../y/ComponentManager.h"
#include "../../../meta.h"


GeometryRendererGL::GeometryRendererGL(RenderPathType type, Renderer *parent) : GeometryRenderer(type, parent) {

	vb_fx = new nix::VertexBuffer("3f,4f,2f");

	shader_fx = ResourceManager::load_shader("forward/3d-fx.shader");

	cam = cam_main;
}

void GeometryRendererGL::prepare() {
	PerformanceMonitor::begin(channel);

	cam = cam_main;

	prepare_instanced_matrices();

	PerformanceMonitor::end(channel);
}

void GeometryRendererGL::set_material(Material *m, RenderPathType t, ShaderVariant v) {
	auto s = m->get_shader(t, v);
	set_material_x(m, s);
}

void GeometryRendererGL::set_material_x(Material *m, Shader *s) {
	nix::set_shader(s);
	if (using_view_space)
		s->set_floats("eye_pos", &cam->owner->pos.x, 3); // NAH....
	else
		s->set_floats("eye_pos", &vec3::ZERO.x, 3);
	s->set_int("num_lights", num_lights);
	s->set_int("shadow_index", shadow_index);
	for (auto &u: m->uniforms)
		s->set_floats(u.name, u.p, u.size/4);

	if (m->alpha.mode == TransparencyMode::FUNCTIONS)
		nix::set_alpha(m->alpha.source, m->alpha.destination);
	else if (m->alpha.mode == TransparencyMode::COLOR_KEY_HARD)
		nix::set_alpha(nix::Alpha::SOURCE_ALPHA, nix::Alpha::SOURCE_INV_ALPHA);
	else
		nix::disable_alpha();

	nix::set_textures(weak(m->textures));

	nix::set_material(m->albedo, m->roughness, m->metal, m->emission);
}

#if 0
void GeometryRendererGL::set_textures(const Array<Texture*> &tex) {
	auto tt = tex;
	if (tt.num == 0)
		tt.add(tex_white);
	if (tt.num == 1)
		tt.add(tex_white);
	if (tt.num == 2)
		tt.add(tex_white);
	/*tt.add(fb_shadow1->depth_buffer.get());
	tt.add(fb_shadow2->depth_buffer.get());
	tt.add(cube_map.get());*/
	nix::set_textures(tt);
}
#endif

void create_color_quad(VertexBuffer *vb, const rect &d, const rect &s, const color &c) {
	Array<VertexFx> v = {{{d.x1,d.y1,0}, c, s.x1,s.y1},
	                    {{d.x1,d.y2,0}, c, s.x1,s.y2},
	                    {{d.x2,d.y2,0}, c, s.x2,s.y2},
	                    {{d.x1,d.y1,0}, c, s.x1,s.y1},
	                    {{d.x2,d.y2,0}, c, s.x2,s.y2},
	                    {{d.x2,d.y1,0}, c, s.x2,s.y1}};
	vb->update(v);
}


void GeometryRendererGL::draw_particles() {
	PerformanceMonitor::begin(ch_fx);

	nix::set_shader(shader_fx.get());
	nix::set_alpha(nix::Alpha::SOURCE_ALPHA, nix::Alpha::SOURCE_INV_ALPHA);
	nix::set_z(false, true);

	// particles
	auto r = mat4::rotation(cam->owner->ang);
	for (auto g: world.particle_manager->legacy_groups) {
		nix::set_texture(g->texture);

		Array<VertexFx> v;
		for (auto p: g->particles)
			if (p->enabled) {
				auto m = mat4::translation(p->pos) * r * mat4::scale(p->radius, p->radius, p->radius);

				v.add({m * vec3(-1, 1,0), p->col, p->source.x1, p->source.y1});
				v.add({m * vec3( 1, 1,0), p->col, p->source.x2, p->source.y1});
				v.add({m * vec3( 1,-1,0), p->col, p->source.x2, p->source.y2});
				v.add({m * vec3(-1, 1,0), p->col, p->source.x1, p->source.y1});
				v.add({m * vec3( 1,-1,0), p->col, p->source.x2, p->source.y2});
				v.add({m * vec3(-1,-1,0), p->col, p->source.x1, p->source.y2});
			}
		vb_fx->update(v);
		nix::set_model_matrix(mat4::ID);
		nix::draw_triangles(vb_fx);
	}


	auto particle_groups = ComponentManager::get_list_family<ParticleGroup>();
	for (auto g: *particle_groups) {
		nix::set_texture(g->texture);
		Array<VertexFx> v;
		for (auto& p: g->particles)
			if (p.enabled) {
				auto m = mat4::translation(p.pos) * r * mat4::scale(p.radius, p.radius, p.radius);

				v.add({m * vec3(-1, 1,0), p.col, p.source.x1, p.source.y1});
				v.add({m * vec3( 1, 1,0), p.col, p.source.x2, p.source.y1});
				v.add({m * vec3( 1,-1,0), p.col, p.source.x2, p.source.y2});
				v.add({m * vec3(-1, 1,0), p.col, p.source.x1, p.source.y1});
				v.add({m * vec3( 1,-1,0), p.col, p.source.x2, p.source.y2});
				v.add({m * vec3(-1,-1,0), p.col, p.source.x1, p.source.y2});
			}
		vb_fx->update(v);
		nix::set_model_matrix(mat4::ID);
		nix::draw_triangles(vb_fx);
	}

	// beams
	//Array<Vertex1> v = {{v_0, v_0, 0,0}, {v_0, v_0, 0,1}, {v_0, v_0, 1,1}, {v_0, v_0, 0,0}, {v_0, v_0, 1,1}, {v_0, v_0, 1,0}};
	nix::set_model_matrix(mat4::ID);
	for (auto g: world.particle_manager->legacy_groups) {
		nix::set_texture(g->texture);

		Array<VertexFx> v;

		for (auto p: g->beams) {
			if (!p->enabled)
				continue;
			// TODO geometry shader!
			auto pa = cam->project(p->pos);
			auto pb = cam->project(p->pos + p->length);
			auto pe = vec3::cross(pb - pa, vec3::EZ).normalized();
			auto uae = cam->unproject(pa + pe * 0.1f);
			auto ube = cam->unproject(pb + pe * 0.1f);
			auto _e1 = (p->pos - uae).normalized() * p->radius;
			auto _e2 = (p->pos + p->length - ube).normalized() * p->radius;
			//vec3 e1 = -vec3::cross(cam->ang * vec3::EZ, p->length).normalized() * p->radius/2;

			vec3 p00 = p->pos - _e1;
			vec3 p01 = p->pos - _e2 + p->length;
			vec3 p10 = p->pos + _e1;
			vec3 p11 = p->pos + _e2 + p->length;

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


	nix::set_z(true, true);
	nix::disable_alpha();
	break_point();
	PerformanceMonitor::end(ch_fx);
}

void GeometryRendererGL::draw_skyboxes() {
	nix::set_z(false, false);
	nix::set_cull(nix::CullMode::NONE);
	nix::set_view_matrix(mat4::rotation(cam->owner->ang).transpose());
	for (auto *sb: world.skybox) {
		sb->_matrix = mat4::rotation(sb->owner->ang);
		nix::set_model_matrix(sb->_matrix * mat4::scale(10,10,10));
		for (int i=0; i<sb->material.num; i++) {
			set_material(sb->material[i], type, ShaderVariant::DEFAULT);
			nix::draw_triangles(sb->mesh[0]->sub[i].vertex_buffer);
		}
	}
	nix::set_cull(nix::CullMode::DEFAULT);
	nix::disable_alpha();
	break_point();
}

void GeometryRendererGL::draw_terrains() {
	auto terrains = ComponentManager::get_list_family<Terrain>();
	for (auto *t: *terrains) {
		if (!t->material->cast_shadow and is_shadow_pass())
			continue;
		auto o = t->owner;
		nix::set_model_matrix(mat4::translation(o->pos));
		if (!is_shadow_pass()) {
			set_material(t->material, type, ShaderVariant::DEFAULT);
			auto s = t->material->get_shader(type, ShaderVariant::DEFAULT);
			s->set_floats("pattern0", &t->texture_scale[0].x, 3);
			s->set_floats("pattern1", &t->texture_scale[1].x, 3);
		} else {
			set_material(material_shadow, type, ShaderVariant::DEFAULT);
		}
		t->prepare_draw(cam_main->owner->pos);
		nix::draw_triangles(t->vertex_buffer);
	}
}

void GeometryRendererGL::draw_objects_instanced() {
	for (auto &s: world.sorted_multi) {
		if (!s.material->cast_shadow and is_shadow_pass())
			continue;
		Model *m = s.model;
		nix::set_model_matrix(s.instance->matrices[0]);//m->_matrix);
		if (!is_shadow_pass()) {
			set_material(s.material, type, ShaderVariant::INSTANCED);
		} else {
			set_material(material_shadow, type, ShaderVariant::INSTANCED);
		}
		nix::bind_buffer(5, s.instance->ubo_matrices);
		//msg_write(s.matrices.num);
		nix::draw_instanced_triangles(m->mesh[0]->sub[s.mat_index].vertex_buffer, s.instance->matrices.num);
		//s.material->shader = ss;
	}
}

void GeometryRendererGL::draw_objects_opaque() {
	for (auto &s: world.sorted_opaque) {
		if (!s.material->cast_shadow and is_shadow_pass())
			continue;
		Model *m = s.model;
		m->update_matrix();
		nix::set_model_matrix(m->_matrix);

		auto ani = m->owner ? m->owner->get_component<Animator>() : nullptr;

		if (ani) {
			if (!is_shadow_pass()) {
				set_material(s.material, type, ShaderVariant::ANIMATED);
			} else {
				set_material(material_shadow, type, ShaderVariant::ANIMATED);
			}
			ani->buf->update_array(ani->dmatrix);
			nix::bind_buffer(7, ani->buf);
		} else {
			if (!is_shadow_pass()) {
				set_material(s.material, type, ShaderVariant::DEFAULT);
			} else {
				set_material(material_shadow, type, ShaderVariant::DEFAULT);
			}
		}
		nix::draw_triangles(m->mesh[0]->sub[s.mat_index].vertex_buffer);
	}
}

void GeometryRendererGL::draw_objects_transparent() {
	nix::set_z(false, true);
	if (is_shadow_pass())
		return;
	for (auto &s: world.sorted_trans) {
		Model *m = s.model;
		m->update_matrix();
		nix::set_model_matrix(m->_matrix);
		nix::set_cull(nix::CullMode::NONE);
		auto ani = m->owner ? m->owner->get_component<Animator>() : nullptr;
		if (ani) {
			set_material(s.material, type, ShaderVariant::ANIMATED);
		} else {
			set_material(s.material, type, ShaderVariant::DEFAULT);
		}
		nix::draw_triangles(m->mesh[0]->sub[s.mat_index].vertex_buffer);
		nix::set_cull(nix::CullMode::DEFAULT);
	}
	nix::disable_alpha();
	nix::set_z(true, true);
}

void GeometryRendererGL::draw_user_meshes(bool transparent) {
	auto meshes = ComponentManager::get_list_family<UserMesh>();
	for (auto *m: *meshes) {
		if (m->material->is_transparent() != transparent)
			continue;
		if (!m->material->cast_shadow and is_shadow_pass())
			continue;
		auto o = m->owner;
		nix::set_model_matrix(o->get_matrix());
		if (!is_shadow_pass()) {
			auto shader = user_mesh_shader(m, type);
			set_material_x(m->material, shader);
		} else {
			auto shader = user_mesh_shadow_shader(m, material_shadow, type);
			set_material_x(material_shadow, shader);
		}
		if (m->topology == PrimitiveTopology::TRIANGLES)
			nix::draw_triangles(m->vertex_buffer);
		else if (m->topology == PrimitiveTopology::POINTS)
			nix::draw_points(m->vertex_buffer);
		else if (m->topology == PrimitiveTopology::LINES)
			nix::draw_points(m->vertex_buffer);
	}
}


void GeometryRendererGL::prepare_instanced_matrices() {
	PerformanceMonitor::begin(ch_pre);
	for (auto &s: world.sorted_multi) {
		if (!s.instance->ubo_matrices)
			s.instance->ubo_matrices = new nix::UniformBuffer();
		s.instance->ubo_matrices->update_array(s.instance->matrices);
	}
	PerformanceMonitor::end(ch_pre);
}



void GeometryRendererGL::draw_opaque() {
	if (!is_shadow_pass()) {
		nix::set_z(true, true);
		nix::set_view_matrix(cam->view_matrix());
		nix::bind_buffer(1, ubo_light);
		nix::bind_texture(3, fb_shadow1->depth_buffer.get());
		nix::bind_texture(4, fb_shadow2->depth_buffer.get());
		nix::bind_texture(5, cube_map.get());
	}

	// opaque
	draw_terrains();
	draw_objects_instanced();
	draw_objects_opaque();
	draw_user_meshes(false);
}

void GeometryRendererGL::draw_transparent() {
	nix::set_view_matrix(cam->view_matrix());
	//nix::set_z(true, true);

	nix::bind_buffer(1, ubo_light);
	nix::bind_texture(3, fb_shadow1->depth_buffer.get());
	nix::bind_texture(4, fb_shadow2->depth_buffer.get());
	nix::bind_texture(5, cube_map.get());

	draw_objects_transparent();
	draw_user_meshes(true);
	draw_particles();
}


#endif
