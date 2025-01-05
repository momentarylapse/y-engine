/*
 * ShadowRenderer.cpp
 *
 *  Created on: Dec 11, 2022
 *      Author: michi
 */

#include "ShadowRenderer.h"

#ifdef USING_OPENGL
#include "../geometry/GeometryRendererGL.h"
#include "../../target/TextureRendererGL.h"
#else
#include "../geometry/GeometryRendererVulkan.h"
#include "../../target/TextureRendererVulkan.h"
#endif
#include <renderer/path/RenderPath.h>
#include "../WorldRenderer.h"
#include "../../base.h"
#include "../../../graphics-impl.h"
#include "../../../helper/PerformanceMonitor.h"
#include <world/Material.h>
#include <world/Camera.h>
#include "../../../Config.h"


ShadowRenderer::Cascade::Cascade() = default;
ShadowRenderer::Cascade::~Cascade() = default;

ShadowRenderer::ShadowRenderer() :
		RenderTask("shdw")
{
	//int shadow_box_size = config.get_float("shadow.boxsize", 2000);
	int shadow_resolution = config.get_int("shadow.resolution", 1024);

	material = new Material(resource_manager);
	material->pass0.shader_path = "shadow.shader";

	for (int i=0; i<NUM_CASCADES; i++) {
		auto& c = cascades[i];
		c.geo_renderer = new GeometryRenderer(RenderPathType::Forward, scene_view);
		c.geo_renderer->flags = GeometryRenderer::Flags::SHADOW_PASS;
		c.geo_renderer->material_shadow = material.get();

		shared tex = new Texture(shadow_resolution, shadow_resolution, "rgba:i8");
		c.depth_buffer = new DepthBuffer(shadow_resolution, shadow_resolution, "d:f32");
		c.texture_renderer = new TextureRenderer("cascd", {tex, c.depth_buffer}, {"autoclear"});
		c.texture_renderer->use_params_area = false;
		c.scale = (i == 0) ? 4.0f : 1.0f;
		c.texture_renderer->add_child(c.geo_renderer.get());
	}
}

void ShadowRenderer::set_scene(SceneView &parent_scene_view) {
	scene_view.cam = parent_scene_view.cam;
	scene_view.lights = parent_scene_view.lights;
	scene_view.shadow_index = parent_scene_view.shadow_index;
}

void ShadowRenderer::render(const RenderParams& params) {
	PerformanceMonitor::begin(ch_prepare);
	gpu_timestamp_begin(params, ch_prepare);

	render_cascade(params, cascades[0]);
	render_cascade(params, cascades[1]);

	gpu_timestamp_end(params, ch_prepare);
	PerformanceMonitor::end(ch_prepare);
}

void ShadowRenderer::render_cascade(const RenderParams& _params, Cascade& c) {
	auto params = _params.with_target(c.texture_renderer->frame_buffer.get());
	params.desired_aspect_ratio = 1.0f;
	auto& rvd = c.geo_renderer->cur_rvd;
	c.geo_renderer->prepare(params);

	rvd.begin_scene(&scene_view);
	proj = rvd.shadow_proj;

#ifdef USING_OPENGL
	auto m = mat4::scale(c.scale, c.scale, 1);
#else
	auto m = mat4::scale(c.scale, -c.scale, 1);
	rvd.index = 0;
#endif
	rvd.scene_view = &scene_view;
	rvd.ubo.num_lights = 0;
	rvd.ubo.shadow_index = -1;
	//m = m * jitter(sfb->width*8, sfb->height*8, 1);
	rvd.set_projection_matrix(m * proj);
	rvd.set_view_matrix(mat4::ID);


	// all opaque meshes
	c.geo_renderer->set(GeometryRenderer::Flags::SHADOW_PASS);
	c.texture_renderer->render(params);
}
