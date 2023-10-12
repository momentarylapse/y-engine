/*
 * WorldRendererGLForward.cpp
 *
 *  Created on: Jun 2, 2021
 *      Author: michi
 */

#include "WorldRendererGLForward.h"
#include "pass/ShadowRendererGL.h"
#include "geometry/GeometryRendererGL.h"

#include <GLFW/glfw3.h>
#ifdef USING_OPENGL
#include "../base.h"
#include "../helper/jitter.h"
#include "../../lib/nix/nix.h"
#include "../../lib/image/image.h"
#include "../../lib/os/msg.h"

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


WorldRendererGLForward::WorldRendererGLForward(Renderer *parent, Camera *cam) : WorldRendererGL("world", parent, cam, RenderPathType::FORWARD) {

	resource_manager->default_shader = "default.shader";
	if (config.get_str("renderer.shader-quality", "pbr") == "pbr") {
		resource_manager->load_shader("module-lighting-pbr.shader");
		resource_manager->load_shader("forward/module-surface-pbr.shader");
	} else {
		resource_manager->load_shader("forward/module-surface.shader");
	}
	resource_manager->load_shader("module-vertex-default.shader");
	resource_manager->load_shader("module-vertex-animated.shader");
	resource_manager->load_shader("module-vertex-instanced.shader");
	resource_manager->load_shader("module-vertex-lines.shader");
	resource_manager->load_shader("module-vertex-points.shader");
	resource_manager->load_shader("module-vertex-fx.shader");
	resource_manager->load_shader("module-geometry-lines.shader");
	resource_manager->load_shader("module-geometry-points.shader");

	create_more();
}

void WorldRendererGLForward::prepare(const RenderParams& params) {
	PerformanceMonitor::begin(channel);

	if (!cam)
		cam = cam_main;
	geo_renderer->cam = cam;
	shadow_renderer->cam = cam;

	static int _frame = 0;
	_frame ++;
	if (_frame > 10) {
		if (world.ego)
			render_into_cubemap(depth_cube.get(), cube_map.get(), world.ego->pos);
		_frame = 0;
	}

	prepare_lights();
	geo_renderer->prepare(params);

	if (shadow_index >= 0)
		shadow_renderer->render(shadow_proj);

	PerformanceMonitor::end(channel);
}

void WorldRendererGLForward::draw(const RenderParams& params) {
	PerformanceMonitor::begin(channel);

	auto fb = frame_buffer();
	bool flip_y = params.target_is_window;

	PerformanceMonitor::begin(ch_bg);

	auto m = flip_y ? mat4::scale(1,-1,1) : mat4::ID;
	if (config.antialiasing_method == AntialiasingMethod::TAA)
		 m *= jitter(fb->width, fb->height, 0);

	// skyboxes
	float max_depth = cam->max_depth;
	cam->max_depth = 2000000;
	cam->update_matrices(params.desired_aspect_ratio);
	nix::set_projection_matrix(m * cam->m_projection);

	nix::clear_color(world.background);
	nix::clear_z();
	nix::set_cull(flip_y ? nix::CullMode::CCW : nix::CullMode::CW);
	nix::set_wire(wireframe);

	geo_renderer->draw_skyboxes();
	PerformanceMonitor::end(ch_bg);


	// world
	PerformanceMonitor::begin(ch_world);
	cam->max_depth = max_depth;
	cam->update_matrices(params.desired_aspect_ratio);
	nix::set_projection_matrix(m * cam->m_projection);

	nix::bind_buffer(1, ubo_light);
	nix::set_view_matrix(cam->view_matrix());
	nix::set_z(true, true);
	nix::set_cull(flip_y ? nix::CullMode::CCW : nix::CullMode::CW);

	geo_renderer->draw_opaque();
	geo_renderer->draw_transparent();
	break_point();
	PerformanceMonitor::end(ch_world);

	//nix::set_scissor(rect::EMPTY);

	nix::set_cull(nix::CullMode::DEFAULT);
	nix::set_wire(false);

	PerformanceMonitor::end(channel);
}

#if 0
void WorldRendererGLForward::render_into_texture(FrameBuffer *fb, Camera *cam) {
	//draw();


	prepare_lights(cam);

	if (shadow_index >= 0) {
		render_shadow_map(fb_shadow1.get(), 4);
		render_shadow_map(fb_shadow2.get(), 1);
	}

	bool flip_y = false;
	nix::bind_frame_buffer(fb);

	PerformanceMonitor::begin(ch_bg);

	auto m = flip_y ? mat4::scale(1,-1,1) : mat4::ID;
	if (config.antialiasing_method == AntialiasingMethod::TAA)
		 m *= jitter(fb->width, fb->height, 0);

	// skyboxes
	float max_depth = cam->max_depth;
	cam->max_depth = 2000000;
	cam->update_matrices((float)fb->width / (float)fb->height);
	nix::set_projection_matrix(m * cam->m_projection);

	nix::clear_color(world.background);
	nix::clear_z();
	nix::set_cull(flip_y ? nix::CullMode::CCW : nix::CullMode::CW);

	draw_skyboxes(cam);
	PerformanceMonitor::end(ch_bg);


	// world
	PerformanceMonitor::begin(ch_world);
	cam->max_depth = max_depth;
	cam->update_matrices((float)fb->width / (float)fb->height);
	nix::set_projection_matrix(m * cam->m_projection);

	nix::bind_buffer(1, ubo_light);
	nix::set_view_matrix(cam->view_matrix());
	nix::set_z(true, true);
	nix::set_cull(flip_y ? nix::CullMode::CCW : nix::CullMode::CW);

	draw_world();
	Scheduler::handle_render_inject();
	break_point();
	PerformanceMonitor::end(ch_world);

	draw_particles(cam);
	//nix::set_scissor(rect::EMPTY);

	nix::set_cull(nix::CullMode::DEFAULT);
}
#endif

#endif
