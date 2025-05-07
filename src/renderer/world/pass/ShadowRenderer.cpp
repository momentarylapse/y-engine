/*
 * ShadowRenderer.cpp
 *
 *  Created on: Dec 11, 2022
 *      Author: michi
 */

#include "ShadowRenderer.h"
#include "../geometry/GeometryRenderer.h"
#include "../../target/TextureRenderer.h"
#include <lib/os/msg.h>
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

ShadowRenderer::ShadowRenderer(Camera* cam, shared_array<MeshEmitter> emitters) :
		RenderTask("shdw")
{
	//int shadow_box_size = config.get_float("shadow.boxsize", 2000);
	int shadow_resolution = config.get_int("shadow.resolution", 1024);

	material = new Material(resource_manager);
	material->pass0.shader_path = "shadow.shader";

	scene_view.cam = cam;
	scene_view.shadow_indices.clear();

	for (int i=0; i<NUM_CASCADES; i++) {
		auto& c = cascades[i];
		c.scene_renderer = new SceneRenderer(scene_view);
		c.scene_renderer->is_shadow_pass = true;
		c.scene_renderer->rvd.material_shadow = material.get();
		for (auto e: weak(emitters))
			c.scene_renderer->add_emitter(e);

		shared tex = new Texture(shadow_resolution, shadow_resolution, "rgba:i8");
		c.depth_buffer = new DepthBuffer(shadow_resolution, shadow_resolution, "d:f32");
		c.texture_renderer = new TextureRenderer(format("cas%d", i), {tex, c.depth_buffer}, {"autoclear"});
		c.scale = (i == 0) ? 4.0f : 1.0f;
		c.texture_renderer->add_child(c.scene_renderer.get());
	}
}

void ShadowRenderer::set_projection(const mat4& proj) {
	for (int i=0; i<NUM_CASCADES; i++) {
		auto& c = cascades[i];

#ifdef USING_OPENGL
		auto m = mat4::scale(c.scale, c.scale, 1);
#else
		auto m = mat4::scale(c.scale, -c.scale, 1);
#endif
		c.scene_renderer->set_view(vec3::ZERO, quaternion::ID, m * proj);
	}
}

void ShadowRenderer::render(const RenderParams& params) {
	PerformanceMonitor::begin(channel);
	gpu_timestamp_begin(params, channel);

	render_cascade(params, cascades[0]);
	render_cascade(params, cascades[1]);

	gpu_timestamp_end(params, channel);
	PerformanceMonitor::end(channel);
}

void ShadowRenderer::render_cascade(const RenderParams& _params, Cascade& c) {
	auto params = _params.with_target(c.texture_renderer->frame_buffer.get());
	params.desired_aspect_ratio = 1.0f;

	// all opaque meshes
	c.texture_renderer->render(params);
}
