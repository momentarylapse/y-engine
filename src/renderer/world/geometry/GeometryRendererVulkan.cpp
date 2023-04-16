/*
 * GeometryRendererVulkan.cpp
 *
 *  Created on: Dec 16, 2022
 *      Author: michi
 */

#include "GeometryRendererVulkan.h"

#include <GLFW/glfw3.h>
#ifdef USING_VULKAN
#include "../WorldRendererVulkan.h"
#include "../../helper/PipelineManager.h"
#include "../../base.h"
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



const int LOCATION_PARAMS = 0;
const int LOCATION_LIGHT = 1;
const int LOCATION_SHADOW0 = 2;
const int LOCATION_SHADOW1 = 3;
const int LOCATION_CUBE = 5;
const int LOCATION_TEX0 = 4;
const int LOCATION_INSTANCE_MATRICES = 7;
const int LOCATION_FX_TEX0 = 1;

const int MAX_LIGHTS = 1024;
const int MAX_INSTANCES = 1<<11;


GeometryRendererVulkan::GeometryRendererVulkan(RenderPathType type, Renderer *parent) : GeometryRenderer(type, parent) {

	vb_fx = new VertexBuffer("3f,4f,2f");

	cam = cam_main;



	rvd_def.ubo_light = new UniformBuffer(MAX_LIGHTS * sizeof(UBOLight));
	//for (int i=0; i<6; i++)
	//	rvd_cube[i].ubo_light = new UniformBuffer(MAX_LIGHTS * sizeof(UBOLight));


	shader_fx = ResourceManager::load_shader("vulkan/3d-fx.shader");
	pipeline_fx = new vulkan::GraphicsPipeline(shader_fx.get(), render_pass(), 0, "triangles", "3f,4f,2f");
	pipeline_fx->set_blend(Alpha::SOURCE_ALPHA, Alpha::SOURCE_INV_ALPHA);
	pipeline_fx->set_z(true, false);
	pipeline_fx->set_culling(CullMode::NONE);
	pipeline_fx->rebuild();

}

void GeometryRendererVulkan::prepare() {
	PerformanceMonitor::begin(channel);

	cam = cam_main;

	prepare_instanced_matrices();

	PerformanceMonitor::end(channel);
}



GraphicsPipeline* GeometryRendererVulkan::get_pipeline(Shader *s, RenderPass *rp, Material *m, PrimitiveTopology top, VertexBuffer *vb) {
	if (m->alpha.mode == TransparencyMode::FUNCTIONS) {
		return PipelineManager::get_alpha(s, rp, top, vb, m->alpha.source, m->alpha.destination, false);
	} else if (m->alpha.mode == TransparencyMode::COLOR_KEY_HARD) {
		return PipelineManager::get_alpha(s, rp, top, vb, Alpha::SOURCE_ALPHA, Alpha::SOURCE_INV_ALPHA, true);
	} else {
		return PipelineManager::get(s, rp, top, vb);
	}
}

void GeometryRendererVulkan::set_material(CommandBuffer *cb, RenderPass *rp, DescriptorSet *dset, Material *m, RenderPathType t, ShaderVariant v, PrimitiveTopology top, VertexBuffer *vb) {
	auto s = m->get_shader(t, v);
	auto p = get_pipeline(s, rp, m, top, vb);
	set_material_x(cb, rp, dset, m, p);
}

void GeometryRendererVulkan::set_material_x(CommandBuffer *cb, RenderPass *rp, DescriptorSet *dset, Material *m, GraphicsPipeline *p) {
	cb->bind_pipeline(p);

	set_textures(dset, LOCATION_TEX0, max(m->textures.num, 1), weak(m->textures));
	dset->update();
	cb->bind_descriptor_set(0, dset);


	/*nix::set_shader(s);
	if (using_view_space)
		s->set_floats("eye_pos", &cam->owner->pos.x, 3);
	else
		s->set_floats("eye_pos", &vec3::ZERO.x, 3);
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

	nix::set_material(m->albedo, m->roughness, m->metal, m->emission);*/
}

void GeometryRendererVulkan::set_textures(DescriptorSet *dset, int i0, int n, const Array<Texture*> &tex) {
	/*auto tt = tex;
	if (tt.num == 0)
		tt.add(tex_white.get());
	if (tt.num == 1)
		tt.add(tex_white.get());
	if (tt.num == 2)
		tt.add(tex_white.get());
	//tt.add(fb_shadow->depth_buffer.get());
	//tt.add(fb_shadow2->depth_buffer.get());
	//tt.add(cube_map.get());
	foreachi (auto t, tt, i)
		dset->set_texture(i0 + i, t);*/
	for (int k=0; k<n; k++) {
		dset->set_texture(i0 + k, tex_white);
		if (k < tex.num)
			if (tex[k])
				dset->set_texture(i0 + k, tex[k]);
	}
	if (fb_shadow1)
		dset->set_texture(LOCATION_SHADOW0, fb_shadow1->attachments[1].get());
	if (fb_shadow1)
		dset->set_texture(LOCATION_SHADOW1, fb_shadow2->attachments[1].get());
	if (cube_map)
		dset->set_texture(LOCATION_CUBE, cube_map.get());
}




void GeometryRendererVulkan::draw_particles(CommandBuffer *cb, RenderPass *rp, Camera *cam, RenderViewDataVK &rvd) {
	PerformanceMonitor::begin(ch_fx);
	auto &rda = rvd.rda_fx;

	cb->bind_pipeline(pipeline_fx);

	UBOFx ubo;
	ubo.p = cam->m_projection;
	ubo.v = cam->m_view;
	ubo.m = mat4::ID;

	// particles
	auto r = mat4::rotation(cam->owner->ang);
	int index = 0;
	for (auto g: world.particle_manager->legacy_groups) {

		if (index >= rda.num) {
			rda.add({new UniformBuffer(sizeof(UBOFx)),
				pool->create_set(shader_fx.get()),
				new VertexBuffer("3f,4f,2f")});
			//rda[index].dset->set_buffer(LOCATION_LIGHT, ubo_light);
			rda[index].dset->set_buffer(LOCATION_PARAMS, rda[index].ubo);
			rda[index].dset->set_texture(LOCATION_FX_TEX0, g->texture);
			rda[index].dset->update();
		}

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
		rda[index].vb->update(v);

		rda[index].ubo->update(&ubo);

		cb->bind_descriptor_set(0, rda[index].dset);
		cb->draw(rda[index].vb);

		index ++;
	}

	// beams
	for (auto g: world.particle_manager->legacy_groups) {

		if (index >= rda.num) {
			rda.add({new UniformBuffer(sizeof(UBOFx)),
				pool->create_set(shader_fx.get()),
				new VertexBuffer("3f,4f,2f")});
			//rda[index].dset->set_buffer(LOCATION_LIGHT, ubo_light);
			rda[index].dset->set_buffer(LOCATION_PARAMS, rda[index].ubo);
			rda[index].dset->set_texture(LOCATION_FX_TEX0, g->texture);
			rda[index].dset->update();
		}

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

		rda[index].vb->update(v);

		rda[index].ubo->update(&ubo);

		cb->bind_descriptor_set(0, rda[index].dset);
		cb->draw(rda[index].vb);

		index ++;
	}


	PerformanceMonitor::end(ch_fx);
}

void GeometryRendererVulkan::draw_skyboxes(CommandBuffer *cb, RenderPass *rp, Camera *cam, float aspect, RenderViewDataVK &rvd) {
	auto &rda = rvd.rda_sky;

	int index = 0;
	UBO ubo;

	float max_depth = cam->max_depth;
	cam->max_depth = 2000000;
	cam->update_matrices(aspect);

	ubo.p = cam->m_projection;
	ubo.v = mat4::rotation(cam->owner->ang).transpose();
	ubo.m = mat4::ID;
	ubo.num_lights = world.lights.num;

	for (auto *sb: world.skybox) {
		sb->_matrix = mat4::rotation(sb->owner->ang);
		ubo.m = sb->_matrix * mat4::scale(10,10,10);


		for (int i=0; i<sb->material.num; i++) {
			if (index >= rda.num) {
				rda.add({new UniformBuffer(sizeof(UBO)),
					pool->create_set(sb->material[i]->get_shader(type, ShaderVariant::DEFAULT))});
				rda[index].dset->set_buffer(LOCATION_PARAMS, rda[index].ubo);
				rda[index].dset->set_buffer(LOCATION_LIGHT, rvd.ubo_light);
			}
			ubo.albedo = sb->material[i]->albedo;
			ubo.emission = sb->material[i]->emission;
			ubo.metal = sb->material[i]->metal;
			ubo.roughness = sb->material[i]->roughness;

			rda[index].ubo->update(&ubo);

			auto vb = sb->mesh[0]->sub[i].vertex_buffer;
			set_material(cb, rp, rda[index].dset, sb->material[i], type, ShaderVariant::DEFAULT, PrimitiveTopology::TRIANGLES, vb);
			cb->draw(vb);

			index ++;
		}
	}


	cam->max_depth = max_depth;
	cam->update_matrices(aspect);
}

void GeometryRendererVulkan::draw_terrains(CommandBuffer *cb, RenderPass *rp, UBO &ubo, RenderViewDataVK &rvd) {
	auto &rda = rvd.rda_tr;
	int index = 0;

	ubo.m = mat4::ID;

	auto terrains = ComponentManager::get_list_family<Terrain>();
	for (auto *t: *terrains) {
		auto o = t->owner;
		ubo.m = mat4::translation(o->pos);
		ubo.albedo = t->material->albedo;
		ubo.emission = t->material->emission;
		ubo.metal = t->material->metal;
		ubo.roughness = t->material->roughness;

		if (index >= rda.num) {
			rda.add({new UniformBuffer(sizeof(UBO)),
				pool->create_set(t->material->get_shader(type, ShaderVariant::DEFAULT))});
			rda[index].dset->set_buffer(LOCATION_PARAMS, rda[index].ubo);
			rda[index].dset->set_buffer(LOCATION_LIGHT, rvd.ubo_light);
		}

		rda[index].ubo->update(&ubo);

		if (!is_shadow_pass()) {
			set_material(cb, rp, rda[index].dset, t->material, type, ShaderVariant::DEFAULT, PrimitiveTopology::TRIANGLES, t->vertex_buffer);
			cb->push_constant(0, 4, &t->texture_scale[0].x);
			cb->push_constant(4, 4, &t->texture_scale[1].x);
		} else {
			set_material(cb, rp, rda[index].dset, material_shadow, type, ShaderVariant::DEFAULT, PrimitiveTopology::TRIANGLES, t->vertex_buffer);
		}
		t->prepare_draw(cam_main->owner->pos);
		cb->draw(t->vertex_buffer);
		index ++;
	}
}

void GeometryRendererVulkan::draw_objects_instanced(CommandBuffer *cb, RenderPass *rp, UBO &ubo, RenderViewDataVK &rvd) {
	auto &rda = rvd.rda_ob_multi;
	int index = 0;
	ubo.m = mat4::ID;

	for (auto &s: world.sorted_multi) {
		if (!s.material->cast_shadow and is_shadow_pass())
			continue;
		Model *m = s.model;

		if (index >= rda.num) {
			rda.add({new UniformBuffer(sizeof(UBO)),
				pool->create_set(s.material->get_shader(type, ShaderVariant::INSTANCED))});
			rda[index].dset->set_buffer(LOCATION_PARAMS, rda[index].ubo);
			rda[index].dset->set_buffer(LOCATION_LIGHT, rvd.ubo_light);
			rda[index].dset->set_buffer(LOCATION_INSTANCE_MATRICES, s.instance->ubo_matrices);
		}

		m->update_matrix();
		//ubo.m = m->_matrix;
		ubo.albedo = s.material->albedo;
		ubo.emission = s.material->emission;
		ubo.metal = s.material->metal;
		ubo.roughness = s.material->roughness;
		rda[index].ubo->update_part(&ubo, 0, sizeof(UBO));

		auto vb = m->mesh[0]->sub[s.mat_index].vertex_buffer;
		if (!is_shadow_pass())
			set_material(cb, rp, rda[index].dset, s.material, type, ShaderVariant::INSTANCED, PrimitiveTopology::TRIANGLES, vb);
		else
			set_material(cb, rp, rda[index].dset, material_shadow, type, ShaderVariant::INSTANCED, PrimitiveTopology::TRIANGLES, vb);

		cb->draw_instanced(vb, min(s.instance->matrices.num, MAX_INSTANCES));
		index ++;
	}
}

void GeometryRendererVulkan::draw_objects_opaque(CommandBuffer *cb, RenderPass *rp, UBO &ubo, RenderViewDataVK &rvd) {
	auto &rda = rvd.rda_ob;
	int index = 0;

	ubo.m = mat4::ID;

	for (auto &s: world.sorted_opaque) {
		if (!s.material->cast_shadow and is_shadow_pass())
			continue;
		Model *m = s.model;

		auto ani = m->owner ? m->owner->get_component<Animator>() : nullptr;

		if (index >= rda.num) {
			rda.add({new UniformBuffer(ani ? (sizeof(UBO)+sizeof(mat4) * ani->dmatrix.num) : sizeof(UBO)),
				pool->create_set(s.material->get_shader(type, ShaderVariant::DEFAULT))});
			rda[index].dset->set_buffer(LOCATION_PARAMS, rda[index].ubo);
			rda[index].dset->set_buffer(LOCATION_LIGHT, rvd.ubo_light);
		}

		m->update_matrix();
		ubo.m = m->_matrix;
		ubo.albedo = s.material->albedo;
		ubo.emission = s.material->emission;
		ubo.metal = s.material->metal;
		ubo.roughness = s.material->roughness;
		rda[index].ubo->update_part(&ubo, 0, sizeof(UBO));
		if (ani)
			rda[index].ubo->update_array(ani->dmatrix, sizeof(UBO));

		auto vb = m->mesh[0]->sub[s.mat_index].vertex_buffer;
		if (ani) {
			if (!is_shadow_pass())
				set_material(cb, rp, rda[index].dset, s.material, type, ShaderVariant::ANIMATED, PrimitiveTopology::TRIANGLES, vb);
			else
				set_material(cb, rp, rda[index].dset, material_shadow, type, ShaderVariant::ANIMATED, PrimitiveTopology::TRIANGLES, vb);
		} else {
			if (!is_shadow_pass())
				set_material(cb, rp, rda[index].dset, s.material, type, ShaderVariant::DEFAULT, PrimitiveTopology::TRIANGLES, vb);
			else
				set_material(cb, rp, rda[index].dset, material_shadow, type, ShaderVariant::DEFAULT, PrimitiveTopology::TRIANGLES, vb);
		}

		cb->draw(vb);
		index ++;
	}
}

void GeometryRendererVulkan::draw_objects_transparent(CommandBuffer *cb, RenderPass *rp, UBO &ubo, RenderViewDataVK &rvd) {
	auto &rda = rvd.rda_ob_trans;
	int index = 0;

	ubo.m = mat4::ID;

	for (auto &s: world.sorted_trans) {
		//if (!s.material->cast_shadow)
		//	continue;
		Model *m = s.model;

		auto ani = m->owner ? m->owner->get_component<Animator>() : nullptr;

		if (index >= rda.num) {
			rda.add({new UniformBuffer(ani ? (sizeof(UBO)+sizeof(mat4) * ani->dmatrix.num) : sizeof(UBO)),
				pool->create_set(s.material->get_shader(type, ShaderVariant::DEFAULT))});
			rda[index].dset->set_buffer(LOCATION_PARAMS, rda[index].ubo);
			rda[index].dset->set_buffer(LOCATION_LIGHT, rvd.ubo_light);
		}

		m->update_matrix();
		ubo.m = m->_matrix;
		ubo.albedo = s.material->albedo;
		ubo.emission = s.material->emission;
		ubo.metal = s.material->metal;
		ubo.roughness = s.material->roughness;
		rda[index].ubo->update_part(&ubo, 0, sizeof(UBO));
		if (ani)
			rda[index].ubo->update_array(ani->dmatrix, sizeof(UBO));

		auto vb = m->mesh[0]->sub[s.mat_index].vertex_buffer;
		if (ani) {
			set_material(cb, rp, rda[index].dset, s.material, type, ShaderVariant::ANIMATED, PrimitiveTopology::TRIANGLES, vb);
		} else {
			set_material(cb, rp, rda[index].dset, s.material, type, ShaderVariant::DEFAULT, PrimitiveTopology::TRIANGLES, vb);
		}

		cb->draw(vb);
		index ++;
	}
}

void GeometryRendererVulkan::draw_user_meshes(CommandBuffer *cb, RenderPass *rp, UBO &ubo, bool transparent, RenderViewDataVK &rvd) {
	auto &rda = rvd.rda_user;
	int index = 0;

	ubo.m = mat4::ID;

	auto meshes = ComponentManager::get_list_family<UserMesh>();

	for (auto m: *meshes) {
		if (!m->material->cast_shadow and is_shadow_pass())
			continue;
		if (m->material->is_transparent() != transparent)
			continue;

		Shader *shader;
		Material *material;
		if (!is_shadow_pass()) {
			material = m->material;
			shader = user_mesh_shader(m, type);//t);
		} else {
			material = material_shadow;
			shader = user_mesh_shadow_shader(m, material, type);
		}
		auto pipeline = get_pipeline(shader, render_pass(), material, m->topology, m->vertex_buffer);

		if (index >= rda.num) {
			rda.add({new UniformBuffer(sizeof(UBO)),
				pool->create_set(shader)});
			rda[index].dset->set_buffer(LOCATION_PARAMS, rda[index].ubo);
			rda[index].dset->set_buffer(LOCATION_LIGHT, rvd.ubo_light);
		}

		ubo.m = m->owner->get_matrix();
		ubo.albedo = m->material->albedo;
		ubo.emission = m->material->emission;
		ubo.metal = m->material->metal;
		ubo.roughness = m->material->roughness;
		rda[index].ubo->update_part(&ubo, 0, sizeof(UBO));

		if (!is_shadow_pass())
			set_material_x(cb, rp, rda[index].dset, m->material, pipeline);
		else
			set_material_x(cb, rp, rda[index].dset, material_shadow, pipeline);

		cb->draw(m->vertex_buffer);
		index ++;
	}
}


void GeometryRendererVulkan::prepare_instanced_matrices() {
	PerformanceMonitor::begin(ch_pre);
	for (auto &s: world.sorted_multi) {
		if (!s.instance->ubo_matrices)
			s.instance->ubo_matrices = new UniformBuffer(MAX_INSTANCES * sizeof(mat4));
		s.instance->ubo_matrices->update_part(&s.instance->matrices[0], 0, min(s.instance->matrices.num, MAX_INSTANCES) * sizeof(mat4));
	}
	PerformanceMonitor::end(ch_pre);
}



#endif
