//
// Created by Michael Ankele on 2025-05-05.
//

#include "SceneRenderer.h"

#include <helper/PerformanceMonitor.h>
#include <renderer/base.h>
#include <world/Light.h>

#include "../world/geometry/RenderViewData.h"
#include "../world/geometry/SceneView.h"
#include "../../graphics-impl.h"

SceneRenderer::SceneRenderer(RenderPathType type, SceneView& _scene_view) : Renderer("scene"), scene_view(_scene_view) {
	rvd.set_scene_view(&scene_view);
	rvd.type = type;
}

SceneRenderer::~SceneRenderer() = default;

void SceneRenderer::add_emitter(shared<MeshEmitter> emitter) {
	emitters.add(emitter);
}

void SceneRenderer::set_view(const vec3& pos, const quaternion& ang, const mat4& proj) {
	rvd.set_view(pos, ang, proj);
}

void SceneRenderer::set_view_from_camera(const RenderParams& params, Camera* cam) {
	rvd.set_view(params, cam);
}

void SceneRenderer::prepare(const RenderParams& params) {
	PerformanceMonitor::begin(ch_prepare);

	if (!is_shadow_pass)
		rvd.update_light_ubo();
	PerformanceMonitor::end(ch_prepare);
}

void SceneRenderer::draw(const RenderParams& params) {
	PerformanceMonitor::begin(channel);
	gpu_timestamp_begin(params, channel);
	rvd.begin_draw();

	if (background_color)
		rvd.clear(params, {*background_color}, 1.0f);

	if (allow_opaque)
		for (auto e: weak(emitters))
			e->emit(params, rvd, is_shadow_pass);

	if (allow_transparent and !is_shadow_pass)
		for (auto e: weak(emitters))
			e->emit_transparent(params, rvd);

	gpu_timestamp_end(params, channel);
	PerformanceMonitor::end(channel);
}




