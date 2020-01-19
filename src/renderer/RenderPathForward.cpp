/*
 * RenderPathForward.cpp
 *
 *  Created on: Jan 19, 2020
 *      Author: michi
 */

#include "RenderPathForward.h"
#include "Renderer.h"
#include "ShadowMapRenderer.h"
#include "../lib/vulkan/vulkan.h"
#include "../world/world.h"
#include "../world/camera.h"
#include "../fx/Light.h"
#include "../helper/PerformanceMonitor.h"
#include "../gui/Picture.h"

extern vulkan::Texture *tex_white;
extern vulkan::Texture *tex_black;

RenderPathForward::RenderPathForward(Renderer *r, PerformanceMonitor *pm) : RenderPath(r, pm, "forward/3d-shadow.shader") {
	fw_shader_a = vulkan::Shader::load("forward/3d-base.shader");
	fw_pipeline_a = new vulkan::Pipeline(fw_shader_a, renderer->default_render_pass(), 0, 1);
}

RenderPathForward::~RenderPathForward() {
	delete fw_shader_a;
	delete fw_pipeline_a;
}

void RenderPathForward::draw() {
	vulkan::wait_device_idle();
	perf_mon->tick(0);
	light_cam->pos = vector(0,1000,0);
	light_cam->ang = quaternion::rotation_v(vector(pi/2, 0, 0));
	light_cam->zoom = 2;
	light_cam->min_depth = 50;
	light_cam->max_depth = 10000;
	light_cam->set_view(1.0f);
	world.sun->proj = light_cam->m_all;

	prepare_all(shadow_renderer, light_cam);
	render_into_shadow(shadow_renderer);

	if (!renderer->start_frame())
		return;

	perf_mon->tick(1);

	prepare_all(renderer, cam);
	auto r = renderer;
	auto cb = r->cb;
	cb->begin();
	cam->set_view(1.0f);

	//cb->push_constant(112, sizeof(UBOLight), world.sun);

	r->default_render_pass()->clear_color[0] = world.background; // emission
	cb->begin_render_pass(r->default_render_pass(), r->current_frame_buffer());
	cb->set_pipeline(fw_pipeline_a);
	cb->set_viewport(r->area());

	draw_world(cb);
	perf_mon->tick(2);

	gui::render(cb, r->area());
	cb->end_render_pass();
	cb->end();


	renderer->end_frame();
	perf_mon->tick(5);
}


vulkan::DescriptorSet *RenderPathForward::rp_create_dset(const Array<vulkan::Texture*> &tex, vulkan::UBOWrapper *ubo) {
	if (tex.num == 3)
		return new vulkan::DescriptorSet({ubo, world.ubo_light, world.ubo_fog}, {tex[0], tex[1], tex[2], shadow_renderer->depth_buffer});
	else
		return new vulkan::DescriptorSet({ubo, world.ubo_light, world.ubo_fog}, {tex[0], tex_white, tex_black, shadow_renderer->depth_buffer});
}
