/*
 * WorldRendererVulkanForward.cpp
 *
 *  Created on: Nov 18, 2021
 *      Author: michi
 */

#include "WorldRendererVulkanForward.h"
#ifdef USING_VULKAN
#include "pass/ShadowRendererVulkan.h"
#include "../../graphics-impl.h"
#include "../base.h"
#include "../../lib/os/msg.h"

#include "../../helper/PerformanceMonitor.h"
#include "../../helper/ResourceManager.h"
#include "../../helper/Scheduler.h"
#include "../../plugins/PluginManager.h"
#include "../../gui/gui.h"
#include "../../gui/Node.h"
#include "../../gui/Picture.h"
#include "../../gui/Text.h"
#include "../../fx/Particle.h"
#include "../../fx/Beam.h"
#include "../../fx/ParticleManager.h"
#include "../../world/Camera.h"
#include "../../world/Light.h"
#include "../../world/Material.h"
#include "../../world/Model.h"
#include "../../world/Object.h" // meh
#include "../../world/Terrain.h"
#include "../../world/World.h"
#include "../../y/Entity.h"
#include "../../Config.h"
#include "../../meta.h"


WorldRendererVulkanForward::WorldRendererVulkanForward(vulkan::Device *_device, Camera *cam) : WorldRendererVulkan("fw", cam, RenderPathType::FORWARD) {
	device = _device;

	create_more();
}

void WorldRendererVulkanForward::prepare(const RenderParams& params) {
	PerformanceMonitor::begin(ch_prepare);
	if (!scene_view.cam)
		scene_view.cam = cam_main;

	auto cb = params.command_buffer;


	static int _frame = 0;
	_frame ++;
	if (_frame > 10) {
		render_into_cubemap(cb, cube_map.get(), suggest_cube_map_pos());
		_frame = 0;
	}

	scene_view.cam->update_matrices(params.desired_aspect_ratio);

	prepare_lights(scene_view.cam, geo_renderer->rvd_def);
	
	geo_renderer->prepare(params);

	if (scene_view.shadow_index >= 0)
		shadow_renderer->render(cb, scene_view);

	PerformanceMonitor::end(ch_prepare);
}

void WorldRendererVulkanForward::draw(const RenderParams& params) {
	auto cb = params.command_buffer;
	auto rp = params.render_pass;

	PerformanceMonitor::begin(ch_draw);
	gpu_timestamp_begin(cb, ch_draw);

	auto &rvd = geo_renderer->rvd_def;

	geo_renderer->draw_skyboxes(cb, rp, params.desired_aspect_ratio, rvd);

	UBO ubo;
	ubo.p = scene_view.cam->m_projection;
	ubo.v = scene_view.cam->m_view;
	ubo.num_lights = scene_view.lights.num;
	ubo.shadow_index = scene_view.shadow_index;

	geo_renderer->draw_terrains(cb, rp, ubo, rvd);
	geo_renderer->draw_objects_opaque(cb, rp, ubo, rvd);
	geo_renderer->draw_objects_instanced(cb, rp, ubo, rvd);
	geo_renderer->draw_user_meshes(cb, rp, ubo, false, rvd);
	geo_renderer->draw_objects_transparent(cb, rp, ubo, rvd);

	geo_renderer->draw_particles(cb, rp, rvd);
	geo_renderer->draw_user_meshes(cb, rp, ubo, true, rvd);

	gpu_timestamp_end(cb, ch_draw);
	PerformanceMonitor::begin(ch_draw);
}

void WorldRendererVulkanForward::render_into_texture(CommandBuffer *cb, RenderPass *rp, FrameBuffer *fb, Camera *cam, RenderViewDataVK &rvd, const RenderParams& params) {
	rp->clear_color[0] = world.background;

	cb->begin_render_pass(rp, fb);
	cb->set_viewport(rect(0, fb->width, 0, fb->height));

	std::swap(scene_view.cam, cam);
	auto sub_params = params.with_target(fb);
	sub_params.render_pass = rp;
	draw(sub_params);
	std::swap(scene_view.cam, cam);

	cb->end_render_pass();
}

#endif

