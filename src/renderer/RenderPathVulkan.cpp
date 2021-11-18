/*
 * RenderPathVulkan.cpp
 *
 *  Created on: Nov 18, 2021
 *      Author: michi
 */

#include "RenderPathVulkan.h"
#include "../graphics-impl.h"




#ifdef USING_VULKAN

Texture *_tex_white;





#if 0
RenderPathVulkan::RenderPathVulkan(RendererVulkan *r, PerformanceMonitor *pm, const string &shadow_shader_filename, const string &fx_shader_filename) {
	renderer = r;
	perf_mon = pm;


	shader_fx = vulkan::Shader::load(fx_shader_filename);
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


	shadow_renderer = new ShadowMapRenderer(shadow_shader_filename);


	AllowXContainer = false;
	light_cam = new Camera(v_0, quaternion::ID, rect::ID);
	AllowXContainer = true;
}

RenderPathVulkan::~RenderPathVulkan() {
	delete particle_vb;
	delete pipeline_fx;

	delete shadow_renderer;

	delete light_cam;
}

void RenderPathVulkan::pick_shadow_source() {

}

void RenderPathVulkan::draw_world(vulkan::CommandBuffer *cb, int light_index) {

	GeoPush gp;
	gp.eye_pos = cam->pos;

	for (auto *t: world.terrains) {
		gp.model = matrix::ID;
		gp.emission = Black;
		gp.xxx[0] = 0.0f;
		cb->push_constant(0, sizeof(gp), &gp);
		cb->bind_descriptor_set_dynamic(0, t->dset, {light_index});
		cb->draw(t->vertex_buffer);
	}

	for (auto &s: world.sorted_opaque) {
		Model *m = s.model;
		gp.model = mtr(m->pos, m->ang);
		gp.emission = s.material->emission;
		gp.xxx[0] = 0.2f;
		cb->push_constant(0, sizeof(gp), &gp);

		cb->bind_descriptor_set_dynamic(0, s.dset, {light_index});
		cb->draw(m->mesh[0]->sub[0].vertex_buffer);
	}

}

void RenderPathVulkan::prepare_all(Renderer *r, Camera *c) {

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
		//t->dset->set({t->ubo, world.ubo_light, world.ubo_fog}, {t->material->textures[0], tex_white, tex_black, shadow_renderer->depth_buffer});

		t->draw(); // rebuild stuff...
	}
	for (auto &s: world.sorted_opaque) {
		Model *m = s.model;

		u.model = mtr(m->pos, m->ang);
		s.ubo->update(&u);
	}

	gui::update();
}


void RenderPathVulkan::render_into_shadow(ShadowMapRenderer *r) {
	r->start_frame();
	auto *cb = r->cb;

	cb->begin_render_pass(r->default_render_pass(), r->current_frame_buffer());
	cb->set_pipeline(r->pipeline);
	cb->set_viewport(r->area());

	draw_world(cb, 0);
	cb->end_render_pass();

	r->end_frame();
}
#endif

#endif

