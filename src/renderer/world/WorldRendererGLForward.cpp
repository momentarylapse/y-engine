/*
 * WorldRendererGLForward.cpp
 *
 *  Created on: Jun 2, 2021
 *      Author: michi
 */

#include "WorldRendererGLForward.h"
#include "pass/ShadowRenderer.h"
#include "geometry/GeometryRendererGL.h"

#ifdef USING_OPENGL
#include "../base.h"
#include "../helper/jitter.h"
#include "../helper/CubeMapSource.h"
#include <lib/nix/nix.h>
#include <lib/image/image.h>
#include <lib/os/msg.h>
#include <y/ComponentManager.h>

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


WorldRendererGLForward::WorldRendererGLForward(Camera *cam, SceneView& scene_view, RenderViewData& main_rvd) : WorldRenderer("world", cam, scene_view), main_rvd(main_rvd) {
	msg_error("WORLD FORWARD");
	resource_manager->load_shader_module("forward/module-surface.shader");
}

void WorldRendererGLForward::prepare(const RenderParams& params) {
	PerformanceMonitor::begin(ch_prepare);

	PerformanceMonitor::end(ch_prepare);
}

void WorldRendererGLForward::draw(const RenderParams& params) {
	draw_with(params, main_rvd);
}
void WorldRendererGLForward::draw_with(const RenderParams& params, RenderViewData& rvd) {

	PerformanceMonitor::begin(channel);
	gpu_timestamp_begin(params, channel);

	auto fb = params.frame_buffer;
	bool flip_y = params.target_is_window;

	PerformanceMonitor::begin(ch_bg);

	auto m = flip_y ? mat4::scale(1,-1,1) : mat4::ID;
	if (config.antialiasing_method == AntialiasingMethod::TAA)
		 m *= jitter(fb->width, fb->height, 0);

	rvd.begin_scene(&scene_view);

	// skyboxes
	auto cam = scene_view.cam;
	float min_depth = cam->min_depth;
	float max_depth = cam->max_depth;
	cam->min_depth = 1;
	cam->max_depth = 2000000;
	cam->update_matrices(params.desired_aspect_ratio);
	rvd.set_projection_matrix(m * cam->m_projection);

	nix::clear_color(world.background);
	nix::clear_z();
	nix::set_front(flip_y ? nix::Orientation::CW : nix::Orientation::CCW);
	rvd.set_wire(wireframe);

	geo_renderer->set(GeometryRenderer::Flags::ALLOW_SKYBOXES, rvd);
	geo_renderer->draw(params);
	PerformanceMonitor::end(ch_bg);


	// world
	PerformanceMonitor::begin(ch_world);
	cam->max_depth = max_depth;
	cam->min_depth = min_depth;
	cam->update_matrices(params.desired_aspect_ratio);
	rvd.set_projection_matrix(m * cam->m_projection);

	nix::bind_uniform_buffer(1, rvd.ubo_light.get());
	rvd.set_view_matrix(cam->view_matrix());
	rvd.set_z(true, true);
	nix::set_front(flip_y ? nix::Orientation::CW : nix::Orientation::CCW);

	geo_renderer->set(GeometryRenderer::Flags::ALLOW_OPAQUE | GeometryRenderer::Flags::ALLOW_TRANSPARENT, rvd);
	geo_renderer->draw(params);
	PerformanceMonitor::end(ch_world);

	//nix::set_scissor(rect::EMPTY);

	rvd.set_cull(CullMode::BACK);
	nix::set_front(nix::Orientation::CW);
	rvd.set_wire(false);

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

#endif
