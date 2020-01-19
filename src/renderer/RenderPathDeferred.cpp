/*
 * RenderPathDeferred.cpp
 *
 *  Created on: 06.01.2020
 *      Author: michi
 */


#include "RenderPathDeferred.h"

#include "ShadowMapRenderer.h"
#include "GBufferRenderer.h"

#include "../lib/vulkan/vulkan.h"

#include "../meta.h"
#include "../world/world.h"
#include "../world/camera.h"
#include "../world/model.h"
#include "../fx/Light.h"
#include "../lib/math/rect.h"
#include "../gui/Picture.h"
#include "../helper/PerformanceMonitor.h"

#include <iostream>


extern vulkan::Texture *tex_white;
extern vulkan::Texture *tex_black;



RenderPathDeferred::RenderPathDeferred(Renderer *_output_renderer, PerformanceMonitor *pm) : RenderPath(_output_renderer, pm, "3d-shadow2.shader") {
	output_renderer = _output_renderer;
	width = output_renderer->width;
	height = output_renderer->height;
	gbuf_ren = new GBufferRenderer(output_renderer->width, output_renderer->height);


	shader_merge_base = vulkan::Shader::load("2d-gbuf-emission.shader");
	shader_merge_light = vulkan::Shader::load("2d-gbuf-light.shader");
	shader_merge_light_shadow = vulkan::Shader::load("2d-gbuf-light-shadow.shader");
	shader_merge_fog = vulkan::Shader::load("2d-gbuf-fog.shader");

	ubo_x1 = new vulkan::UniformBuffer(sizeof(UBOMatrices));

	_create_dynamic_data();
}
RenderPathDeferred::~RenderPathDeferred() {
	_destroy_dynamic_data();

	delete gbuf_ren;
	delete ubo_x1;

	delete shader_merge_base;
	delete shader_merge_light;
	delete shader_merge_light_shadow;
	delete shader_merge_fog;
}

void RenderPathDeferred::_create_dynamic_data() {


	pipeline_x1 = new vulkan::Pipeline(shader_merge_base, output_renderer->default_render_pass(), 0, 1);
	pipeline_x1->set_z(false, false);
	pipeline_x1->rebuild();

	pipeline_x2 = new vulkan::Pipeline(shader_merge_light, output_renderer->default_render_pass(), 0, 1);
	pipeline_x2->set_dynamic({"scissor"});
	pipeline_x2->set_blend(VK_BLEND_FACTOR_SRC_COLOR, VK_BLEND_FACTOR_ONE);
	pipeline_x2->set_z(false, false);
	pipeline_x2->rebuild();

	pipeline_x2s = new vulkan::Pipeline(shader_merge_light_shadow, output_renderer->default_render_pass(), 0, 1);
	pipeline_x2s->set_dynamic({"scissor"});
	pipeline_x2s->set_blend(VK_BLEND_FACTOR_SRC_COLOR, VK_BLEND_FACTOR_ONE);
	pipeline_x2s->set_z(false, false);
	pipeline_x2s->rebuild();

	pipeline_x3 = new vulkan::Pipeline(shader_merge_fog, output_renderer->default_render_pass(), 0, 1);
	pipeline_x3->set_blend(VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
	pipeline_x3->set_z(false, false);
	pipeline_x3->rebuild();

	dset_x1 = new vulkan::DescriptorSet({ubo_x1, world.ubo_light, world.ubo_fog}, {gbuf_ren->tex_color, gbuf_ren->tex_emission, gbuf_ren->tex_pos, gbuf_ren->tex_normal});
}

void RenderPathDeferred::_destroy_dynamic_data() {
	delete dset_x1;
	delete pipeline_x1;
	delete pipeline_x2;
	delete pipeline_x2s;
	delete pipeline_x3;
}

void RenderPathDeferred::resize(int w, int h) {
	if (w == width and h == height)
		return;

	std::cout << " resize " << w << " x " << h << "\n";
	_destroy_dynamic_data();
	gbuf_ren->resize(w, h);
	_create_dynamic_data();

	width = w;
	height = h;
}

vector RenderPathDeferred::project_pixel(const vector &v) {
	vector p = cam->project(v);
	return vector(p.x * output_renderer->width, (1-p.y) * output_renderer->height, p.z);
}

float RenderPathDeferred::projected_sphere_radius(vector &v, float r) {

	vector p = project_pixel(v);
	float rmax = 0;
	static Array<vector> dirs = {vector::EX, vector::EY, vector::EZ, vector(0.7f, 0.7f, 0), vector(0.7f, 0, 0.7f), vector(0, 0.7f, 0.7f)};
	for (auto &dir: dirs) {
		float f = (p - project_pixel(v + dir * r)).length();
		rmax = max(rmax, f);
	}
	return rmax;
}

rect RenderPathDeferred::light_rect(Light *l) {
	float w = output_renderer->width;
	float h = output_renderer->height;
	if (l->radius < 0)
		return rect(0, w, 0, h);
	vector p = project_pixel(l->pos);
	float r = projected_sphere_radius(l->pos, l->radius);
	if (l->theta < 0)
		r *= 0.10f;//0.17f;
	else
		r *= 0.4f;

	return rect(clampf(p.x - r, 0, w), clampf(p.x + r, 0, w), clampf(p.y - r, 0, h), clampf(p.y + r, 0, h));
}

void RenderPathDeferred::set_light(Light *ll) {
	ll->ubo->update(ll);
	if (!ll->dset)
		ll->dset = new vulkan::DescriptorSet({ubo_x1, ll->ubo, world.ubo_fog}, {gbuf_ren->tex_color, gbuf_ren->tex_emission, gbuf_ren->tex_pos, gbuf_ren->tex_normal, shadow_renderer->depth_buffer});
	else
		ll->dset->set({ubo_x1, ll->ubo, world.ubo_fog}, {gbuf_ren->tex_color, gbuf_ren->tex_emission, gbuf_ren->tex_pos, gbuf_ren->tex_normal, shadow_renderer->depth_buffer});
}



void RenderPathDeferred::draw_from_gbuf_single(vulkan::CommandBuffer *cb, vulkan::Pipeline *pip, vulkan::DescriptorSet *dset, const rect &r = rect::EMPTY) {

	cb->set_pipeline(pip);
	if (r.area() > 0)
		cb->set_scissor(r);

	cb->set_viewport(output_renderer->area());
	if (pip == pipeline_x2s) {
		cb->push_constant(0, 64, &light_cam->m_all);
		cb->push_constant(64, 12, &cam->pos);
	} else {
		cb->push_constant(0, 12, &cam->pos);
	}

	cb->bind_descriptor_set(0, dset);
	cb->draw(Picture::vertex_buffer);
}


void RenderPathDeferred::render_out(vulkan::CommandBuffer *cb, Renderer *ro) {

	UBOMatrices u;
	u.proj = matrix::translation(vector(-1,-1,0)) * matrix::scale(2,2,1);
	u.view = matrix::ID;
	u.model = matrix::ID;
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


void RenderPathDeferred::render_all_from_deferred(Renderer *r) {
	auto *cb = r->cb;
	auto *rp = r->default_render_pass();
	auto *fb = r->current_frame_buffer();
	cur_cam = cam;

	cb->set_viewport(r->area());

	cb->begin_render_pass(rp, fb);

	render_out(cb, r);
	perf_mon->tick(3);
	render_fx(cb, r);
	perf_mon->tick(4);

	gui::render(cb, r->area());

	cb->end_render_pass();

}

void RenderPathDeferred::render_into_gbuffer(GBufferRenderer *r) {
	r->start_frame();
	auto *cb = r->cb;
	cam->set_view(1.0f);

	r->default_render_pass()->clear_color[1] = world.background; // emission
	cb->begin_render_pass(r->default_render_pass(), r->current_frame_buffer());
	cb->set_pipeline(r->pipeline_into_gbuf);
	cb->set_viewport(r->area());

	draw_world(cb, 0);
	cb->end_render_pass();

	r->end_frame();
}


void RenderPathDeferred::draw() {

	vulkan::wait_device_idle();
	//deferred_reenderer->pick_shadow_source();


	resize(renderer->width, renderer->height);

	if (!renderer->start_frame())
		return;

	perf_mon->tick(0);
	light_cam->pos = vector(0,1000,0);
	light_cam->ang = quaternion::rotation_v(vector(pi/2, 0, 0));
	light_cam->zoom = 2;
	light_cam->min_depth = 50;
	light_cam->max_depth = 10000;
	light_cam->set_view(1.0f);

	prepare_all(shadow_renderer, light_cam);
	render_into_shadow(shadow_renderer);
	perf_mon->tick(1);

	prepare_all(gbuf_ren, cam);
	render_into_gbuffer(gbuf_ren);
	perf_mon->tick(2);

	prepare_all(renderer, cam);

	auto cb = renderer->cb;


	cb->begin();
	render_all_from_deferred(renderer);
	cb->end();

	renderer->end_frame();
	perf_mon->tick(5);
}


vulkan::DescriptorSet *RenderPathDeferred::rp_create_dset(const Array<vulkan::Texture*> &tex, vulkan::UniformBuffer *ubo) {
	if (tex.num == 3)
		return new vulkan::DescriptorSet({ubo, world.ubo_light, world.ubo_fog}, tex);
	else
		return new vulkan::DescriptorSet({ubo, world.ubo_light, world.ubo_fog}, {tex[0], tex_white, tex_black});
}
