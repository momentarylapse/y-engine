/*
 * WorldRendererForward.cpp
 *
 *  Created on: Jun 2, 2021
 *      Author: michi
 */

#include "WorldRendererForward.h"
#include "pass/ShadowRenderer.h"
#include "geometry/GeometryRenderer.h"
#include "../base.h"
#include "../helper/jitter.h"
#include "../helper/CubeMapSource.h"
#include "../path/RenderPath.h"
#include <lib/nix/nix.h>
#include <lib/image/image.h>
#include <lib/os/msg.h>
#include <y/ComponentManager.h>
#include <graphics-impl.h>
#include <renderer/x/WorldModelsEmitter.h>
#include <renderer/x/WorldParticlesEmitter.h>
#include <renderer/x/WorldSkyboxEmitter.h>
#include <renderer/x/WorldTerrainsEmitter.h>

#include "../../helper/PerformanceMonitor.h"
#include "../../helper/ResourceManager.h"
#include "../../helper/Scheduler.h"
#include "../../plugins/PluginManager.h"
#include "../../world/Camera.h"
#include "../../world/Light.h"
#include "../../world/World.h"
#include "../../y/Entity.h"
#include "../../Config.h"
#include "../../meta.h"


WorldRendererForward::WorldRendererForward(SceneView& scene_view) : WorldRenderer("world", scene_view) {
	resource_manager->load_shader_module("forward/module-surface.shader");

	scene_renderer = new SceneRenderer(RenderPathType::Forward, scene_view);
	scene_renderer->add_emitter(new WorldSkyboxEmitter);
	scene_renderer->add_emitter(new WorldModelsEmitter);
	scene_renderer->add_emitter(new WorldTerrainsEmitter);
	scene_renderer->add_emitter(new WorldParticlesEmitter);
}

void WorldRendererForward::prepare(const RenderParams& params) {
	PerformanceMonitor::begin(ch_prepare);
	scene_view.cam->update_matrix_cache(params.desired_aspect_ratio);


	scene_renderer->background_color = world.background;
	scene_renderer->set_view_from_camera(params, scene_view.cam);
	scene_renderer->prepare(params);

	PerformanceMonitor::end(ch_prepare);
}

void WorldRendererForward::draw(const RenderParams& params) {
	draw_with(params);
}
void WorldRendererForward::draw_with(const RenderParams& params) {
	PerformanceMonitor::begin(channel);
	gpu_timestamp_begin(params, channel);

	scene_renderer->draw(params);

	gpu_timestamp_end(params, channel);
	PerformanceMonitor::end(channel);
}

#warning "TODO"
#if 0
void WorldRendererGLForward::render_into_texture(FrameBuffer *fb, Camera *cam, RenderViewData &rvd) {
	nix::bind_frame_buffer(fb);

	std::swap(scene_view.cam, cam);
	prepare_lights(cam, rvd);
	draw(RenderParams::into_texture(fb, 1.0f));
	std::swap(scene_view.cam, cam);
}
#endif

