/*
 * ShadowRenderer.cpp
 *
 *  Created on: Dec 11, 2022
 *      Author: michi
 */

#include "ShadowRenderer.h"

#ifdef USING_VULKAN
#include "../WorldRendererVulkan.h"
#include "../../target/TextureRendererVulkan.h"
#include "../geometry/GeometryRendererVulkan.h"
#include "../../base.h"
#include "../../../graphics-impl.h"
#include "../../../world/Light.h"
#include "../../../world/Material.h"
#include "../../../Config.h"
#include <lib/nix/nix.h>
#include <helper/PerformanceMonitor.h>

ShadowRenderer::Cascade::Cascade() = default;
ShadowRenderer::Cascade::~Cascade() = default;

ShadowRenderer::ShadowRenderer() : RenderTask("shdw") {
	int shadow_box_size = config.get_float("shadow.boxsize", 2000);
	int shadow_resolution = config.get_int("shadow.resolution", 1024);

	material = new Material(resource_manager);
	material->pass0.shader_path = "shadow.shader";

	for (int i=0; i<NUM_CASCADES; i++) {
		auto& c = cascades[i];
		auto tex = new Texture(shadow_resolution, shadow_resolution, "rgba:i8");
		auto depth = new DepthBuffer(shadow_resolution, shadow_resolution, "d:f32", true);
		c.texture_renderer = new TextureRenderer({tex, depth}, {"autoclear"});
		c.fb = c.texture_renderer->frame_buffer;
		c.geo_renderer = new GeometryRenderer(RenderPathType::FORWARD, scene_view);
		c.geo_renderer->flags = GeometryRenderer::Flags::SHADOW_PASS;
		c.geo_renderer->material_shadow = material.get();
		add_child(c.geo_renderer.get());
	}
	cascades[0].scale = 4.0f;
	cascades[1].scale = 1.0f;



	//scene_view.ubo_light = new UniformBuffer(8); // dummy
}

void ShadowRenderer::set_scene(SceneView &parent_scene_view) {
	scene_view.cam = parent_scene_view.cam;
	proj = parent_scene_view.shadow_proj;
}

void ShadowRenderer::render(const RenderParams& params) {
	auto cb = params.command_buffer;
	PerformanceMonitor::begin(ch_prepare);
	gpu_timestamp_begin(cb, ch_prepare);

	auto sub_params = RenderParams::WHATEVER;
	sub_params.command_buffer = cb;
	sub_params.render_pass = cascades[0].texture_renderer->render_pass.get();
	render_cascade(sub_params, cascades[0]);
	sub_params.render_pass = cascades[1].texture_renderer->render_pass.get();
	render_cascade(sub_params, cascades[1]);

	gpu_timestamp_end(cb, ch_prepare);
	PerformanceMonitor::end(ch_prepare);
}

void ShadowRenderer::render_cascade(const RenderParams& params, Cascade& c) {
	auto cb = params.command_buffer;
	c.geo_renderer->prepare(RenderParams::into_texture(c.fb.get(), 1.0f));

	auto m = mat4::scale(c.scale, -c.scale, 1);
	//m = m * jitter(sfb->width*8, sfb->height*8, 1);

	c.rvd.index = 0;
	c.rvd.scene_view = &scene_view;
	c.rvd.ubo.num_lights = 0;
	c.rvd.ubo.shadow_index = -1;
	c.rvd.set_projection_matrix(m * proj);
	c.rvd.set_view_matrix(mat4::ID);

	cb->begin_render_pass(params.render_pass, c.fb.get());
	cb->set_viewport(rect(0, c.fb->width, 0, c.fb->height));

	//shadow_pass->set(shadow_proj, scale, &rvd);
	//shadow_pass->draw();

	c.geo_renderer->set(GeometryRenderer::Flags::SHADOW_PASS, c.rvd);
	c.geo_renderer->draw(params);

	cb->end_render_pass();
}

#endif
