//
// Created by Michael Ankele on 2025-05-05.
//

#include "SceneRenderer.h"

#include <helper/PerformanceMonitor.h>
#include <lib/os/msg.h>
#include <renderer/base.h>
#include <world/Light.h>

#include "../world/geometry/RenderViewData.h"
#include "../world/geometry/SceneView.h"
#include "../../graphics-impl.h"
#include "../../world/Camera.h"

SceneRenderer::SceneRenderer(SceneView& _scene_view) : Renderer("scene"), scene_view(_scene_view) {
}

SceneRenderer::~SceneRenderer() = default;

void SceneRenderer::add_emitter(shared<MeshEmitter> emitter) {
	emitters.add(emitter);
}

void SceneRenderer::set_matrices(const mat4& v, const mat4& p) {
	rvd.set_view_matrix(v);
	rvd.set_projection_matrix(p);
}

void SceneRenderer::set_matrices_from_camera(const RenderParams& params, Camera* cam) {
	cam->update_matrices(params.desired_aspect_ratio);
	rvd.set_projection_matrix(cam->m_projection);
	rvd.set_view_matrix(cam->m_view);
}

void SceneRenderer::prepare(const RenderParams& params) {
	PerformanceMonitor::begin(ch_prepare);

	rvd.prepare_scene(&scene_view);

	if (!is_shadow_pass)
		rvd.update_lights();
	PerformanceMonitor::end(ch_prepare);
}

void SceneRenderer::draw(const RenderParams& params) {
	PerformanceMonitor::begin(channel);
	gpu_timestamp_begin(params, channel);
	rvd.begin_draw();

	// FIXME
	/*if (!is_shadow_pass) {
		scene_view.cam->update_matrices(1.3f);
		rvd.update_lights();
	}*/

	if (background_color) {
#ifdef USING_VULKAN
		auto cb = params.command_buffer;
		cb->clear(params.frame_buffer->area(), {*background_color}, 1.0f);
#else
		nix::clear_color(*background_color);
		nix::clear_z();
#endif
	}

	if (allow_opaque)
		for (auto e: weak(emitters))
			e->emit(params, rvd, is_shadow_pass);

	if (allow_transparent and !is_shadow_pass)
		for (auto e: weak(emitters))
			e->emit_transparent(params, rvd);

	gpu_timestamp_end(params, channel);
	PerformanceMonitor::end(channel);
}




