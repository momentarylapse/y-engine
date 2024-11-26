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
#include "../helper/CubeMapSource.h"
#include "../base.h"
#include <lib/os/msg.h>

#include <y/ComponentManager.h>
#include "../../helper/PerformanceMonitor.h"
#include "../../helper/ResourceManager.h"
#include "../../gui/gui.h"
#include "../../gui/Node.h"
#include "../../world/Camera.h"
#include "../../world/World.h"


WorldRendererVulkanForward::WorldRendererVulkanForward(vulkan::Device *_device, Camera *cam) : WorldRendererVulkan("fw", cam, RenderPathType::FORWARD) {
	device = _device;

	resource_manager->load_shader_module("forward/module-surface.shader");
	create_more();
}

void WorldRendererVulkanForward::prepare(const RenderParams& params) {
	PerformanceMonitor::begin(ch_prepare);

	auto cb = params.command_buffer;
	scene_view.check_terrains(cam_main->owner->pos);

	suggest_cube_map_pos();
	auto cube_map_sources = ComponentManager::get_list<CubeMapSource>();
	cube_map_sources.add(cube_map_source);
	for (auto& source: cube_map_sources) {
		if (source->update_rate <= 0)
			continue;
		source->counter ++;
		if (source->counter >= source->update_rate) {
			render_into_cubemap(*source, params);
			source->counter = 0;
		}
	}

	//if (!scene_view.cam)
		scene_view.cam = cam_main;

	scene_view.cam->update_matrices(params.desired_aspect_ratio);

	prepare_lights(scene_view.cam, main_rvd);
	
	geo_renderer->prepare(params);

	if (scene_view.shadow_index >= 0)
		shadow_renderer->render(cb, scene_view);

	PerformanceMonitor::end(ch_prepare);
}

void WorldRendererVulkanForward::draw(const RenderParams& params) {
	draw_with(params, main_rvd);
}

void WorldRendererVulkanForward::draw_with(const RenderParams& params, RenderViewData& rvd) {
	auto cb = params.command_buffer;

	PerformanceMonitor::begin(ch_draw);
	gpu_timestamp_begin(cb, ch_draw);

	rvd.begin_scene(&scene_view);

	rvd.set_projection_matrix(scene_view.cam->m_projection);
	rvd.set_view_matrix(scene_view.cam->m_view);
	rvd.ubo.num_lights = scene_view.lights.num;
	rvd.ubo.shadow_index = scene_view.shadow_index;

	cb->clear(params.frame_buffer->area(), {world.background}, 1.0f);
	geo_renderer->draw_skyboxes(params, rvd);

	geo_renderer->draw_terrains(params, rvd);
	geo_renderer->draw_objects_opaque(params, rvd);

	geo_renderer->draw_objects_instanced(params, rvd);
	geo_renderer->draw_user_meshes(params, false, rvd);

	geo_renderer->draw_objects_transparent(params, rvd);
	geo_renderer->draw_particles(params, rvd);
	geo_renderer->draw_user_meshes(params, true, rvd);

	gpu_timestamp_end(cb, ch_draw);
	PerformanceMonitor::end(ch_draw);
}

void WorldRendererVulkanForward::render_into_texture(Camera *cam, RenderViewData &rvd, const RenderParams& params) {
	auto cb = params.command_buffer;
	auto rp = params.render_pass;
	auto fb = params.frame_buffer;
	rp->clear_color[0] = world.background;

	cb->begin_render_pass(rp, fb);
	cb->set_viewport(fb->area());

	std::swap(scene_view.cam, cam);
	scene_view.cam->update_matrices(params.desired_aspect_ratio); // argh, need more UBOs
	prepare_lights(scene_view.cam, rvd);
	auto sub_params = params.with_target(fb);
	draw_with(sub_params, rvd);
	std::swap(scene_view.cam, cam);

	cb->end_render_pass();
}

#endif

