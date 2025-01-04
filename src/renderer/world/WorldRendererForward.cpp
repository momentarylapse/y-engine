/*
 * WorldRendererForward.cpp
 *
 *  Created on: Jun 2, 2021
 *      Author: michi
 */

#include "WorldRendererForward.h"
#include "pass/ShadowRenderer.h"
#include "geometry/GeometryRendererGL.h"
#include "geometry/GeometryRendererVulkan.h"
#include "../base.h"
#include "../helper/jitter.h"
#include "../helper/CubeMapSource.h"
#include <lib/nix/nix.h>
#include <lib/image/image.h>
#include <lib/os/msg.h>
#include <y/ComponentManager.h>
#include <graphics-impl.h>

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

// https://learnopengl.com/Advanced-OpenGL/Anti-Aliasing


WorldRendererForward::WorldRendererForward(Camera *cam, SceneView& scene_view) : WorldRenderer("world", cam, scene_view) {
	msg_error("WORLD FORWARD");
	resource_manager->load_shader_module("forward/module-surface.shader");
}

void WorldRendererForward::prepare(const RenderParams& params) {
	PerformanceMonitor::begin(ch_prepare);
	scene_view.cam->update_matrices(params.desired_aspect_ratio);

	PerformanceMonitor::end(ch_prepare);
}

void WorldRendererForward::draw(const RenderParams& params) {
	draw_with(params);
}
void WorldRendererForward::draw_with(const RenderParams& params) {

	PerformanceMonitor::begin(channel);
	gpu_timestamp_begin(params, channel);

	auto fb = params.frame_buffer;
	bool flip_y = params.target_is_window;

	PerformanceMonitor::begin(ch_bg);

	auto m = flip_y ? mat4::scale(1,-1,1) : mat4::ID;
//	if (config.antialiasing_method == AntialiasingMethod::TAA)
//		 m *= jitter(fb->width, fb->height, 0);

	auto& rvd = geo_renderer->cur_rvd;

	rvd.begin_scene(&scene_view);

	scene_view.cam->update_matrices(params.desired_aspect_ratio);
	rvd.set_projection_matrix(scene_view.cam->m_projection);
	rvd.set_view_matrix(scene_view.cam->m_view);
	rvd.ubo.num_lights = scene_view.lights.num;
	rvd.ubo.shadow_index = scene_view.shadow_index;

#ifdef USING_VULKAN
	auto cb = params.command_buffer;
	cb->clear(params.frame_buffer->area(), {world.background}, 1.0f);
#else
	nix::clear_color(world.background);
	nix::clear_z();
	nix::set_front(flip_y ? nix::Orientation::CW : nix::Orientation::CCW);
	rvd.set_wire(wireframe);
#endif

	// skyboxes
	geo_renderer->set(GeometryRenderer::Flags::ALLOW_SKYBOXES);
	geo_renderer->draw(params);
	PerformanceMonitor::end(ch_bg);


	// world
	PerformanceMonitor::begin(ch_world);
#ifdef USING_VULKAN
#else
	nix::set_front(flip_y ? nix::Orientation::CW : nix::Orientation::CCW);
#endif

	geo_renderer->set(GeometryRenderer::Flags::ALLOW_OPAQUE | GeometryRenderer::Flags::ALLOW_TRANSPARENT);
	geo_renderer->draw(params);
	PerformanceMonitor::end(ch_world);

#ifdef USING_VULKAN
#else
	rvd.set_cull(CullMode::BACK);
	nix::set_front(nix::Orientation::CW);
	//nix::set_scissor(rect::EMPTY);
	rvd.set_wire(false);
#endif

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

