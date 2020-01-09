/*
 * DeferredRenderer.cpp
 *
 *  Created on: 06.01.2020
 *      Author: michi
 */


#include "DeferredRenderer.h"
#include "ShadowMapRenderer.h"
#include "GBufferRenderer.h"

#include "../meta.h"
#include "../world/world.h"
#include "../world/camera.h"
#include "../fx/Light.h"
#include "../lib/math/rect.h"
#include "../gui/Picture.h"


struct UBOMatrices {
	alignas(16) matrix model;
	alignas(16) matrix view;
	alignas(16) matrix proj;
};


DeferredRenderer::DeferredRenderer(Renderer *_output_renderer) {
	output_renderer = _output_renderer;
	gbuf_ren = new GBufferRenderer();
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
	pipeline_x3->set_blend(VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
	pipeline_x3->set_z(false, false);
	pipeline_x3->rebuild();



	ubo_x1 = new vulkan::UBOWrapper(sizeof(UBOMatrices));
	dset_x1 = new vulkan::DescriptorSet(shader_merge_base->descr_layouts[0], {ubo_x1, world.ubo_light, world.ubo_fog}, {gbuf_ren->tex_color, gbuf_ren->tex_emission, gbuf_ren->tex_pos, gbuf_ren->tex_normal});

	shadow_renderer = new ShadowMapRenderer();

	AllowXContainer = false;
	light_cam = new Camera(v_0, quaternion::ID, rect::ID);
	AllowXContainer = true;
}
DeferredRenderer::~DeferredRenderer() {
	delete shadow_renderer;
	delete gbuf_ren;
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

	delete light_cam;
}


vector DeferredRenderer::project_pixel(const vector &v) {
	vector p = cam->project(v);
	return vector(p.x * output_renderer->width, (1-p.y) * output_renderer->height, p.z);
}

float DeferredRenderer::projected_sphere_radius(vector &v, float r) {

	vector p = project_pixel(v);
	float rmax = 0;
	static Array<vector> dirs = {vector::EX, vector::EY, vector::EZ, vector(0.7f, 0.7f, 0), vector(0.7f, 0, 0.7f), vector(0, 0.7f, 0.7f)};
	for (auto &dir: dirs) {
		float f = (p - project_pixel(v + dir * r)).length();
		rmax = max(rmax, f);
	}
	return rmax;
}

rect DeferredRenderer::light_rect(Light *l) {
	float w = output_renderer->width;
	float h = output_renderer->height;
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

void DeferredRenderer::set_light(Light *ll) {
	UBOLight l;
	l.pos = ll->pos;
	l.dir = ll->dir;
	l.col = ll->col;
	l.radius = ll->radius;
	l.theta = ll->theta;
	l.harshness = ll->harshness;
	ll->ubo->update(&l);
	if (!ll->dset)
		ll->dset = new vulkan::DescriptorSet(shader_merge_light->descr_layouts[0], {ubo_x1, ll->ubo, world.ubo_fog}, {gbuf_ren->tex_color, gbuf_ren->tex_emission, gbuf_ren->tex_pos, gbuf_ren->tex_normal, shadow_renderer->depth_buffer});
	else
		ll->dset->set({ubo_x1, ll->ubo, world.ubo_fog}, {gbuf_ren->tex_color, gbuf_ren->tex_emission, gbuf_ren->tex_pos, gbuf_ren->tex_normal, shadow_renderer->depth_buffer});
}



void DeferredRenderer::draw_from_gbuf_single(vulkan::CommandBuffer *cb, vulkan::Pipeline *pip, vulkan::DescriptorSet *dset, const rect &r = rect::EMPTY) {

	cb->set_pipeline(pip);
	if (r.area() > 0)
		cb->set_scissor(r);

	cb->set_viewport(output_renderer->area());
	if (pip == pipeline_x2s) {
		matrix v = light_cam->m_all.transpose();
		cb->push_constant(0, 64, &v);
		cb->push_constant(64, 12, &cam->pos);
	} else {
		cb->push_constant(0, 12, &cam->pos);
	}

	cb->bind_descriptor_set(0, dset);
	cb->draw(Picture::vertex_buffer);
}


void DeferredRenderer::render_out(vulkan::CommandBuffer *cb, Renderer *ro) {

	UBOMatrices u;
	u.proj = (matrix::translation(vector(-1,-1,0)) * matrix::scale(2,2,1)).transpose();
	u.view = matrix::ID.transpose();
	u.model = matrix::ID.transpose();
	ubo_x1->update(&u);


	// base emission
	draw_from_gbuf_single(cb, pipeline_x1, dset_x1, rect::EMPTY);


	// light passes
	for (auto *l: world.lights) {
		if (l->enabled) {
			set_light(l);

			if (l == world.lights[0])
				draw_from_gbuf_single(cb, pipeline_x2s, l->dset, light_rect(l));
			else
				draw_from_gbuf_single(cb, pipeline_x2, l->dset, light_rect(l));
		}
	}

	// fog
	if (world.fog.enabled)
		draw_from_gbuf_single(cb, pipeline_x3, dset_x1, rect::EMPTY);
}

void DeferredRenderer::DeferredRenderer::pick_shadow_source() {

}

