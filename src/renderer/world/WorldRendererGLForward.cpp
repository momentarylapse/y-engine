/*
 * WorldRendererGLForward.cpp
 *
 *  Created on: Jun 2, 2021
 *      Author: michi
 */

#include "WorldRendererGLForward.h"
#include "pass/ShadowPassGL.h"

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


WorldRendererGLForward::WorldRendererGLForward(Renderer *parent) : WorldRendererGL("world", parent, RenderPathType::FORWARD) {

	fb_shadow1 = new nix::FrameBuffer({
		new nix::Texture(shadow_resolution, shadow_resolution, "rgba:i8"),
		new nix::DepthBuffer(shadow_resolution, shadow_resolution, "d24s8")});
	fb_shadow2 = new nix::FrameBuffer({
		new nix::Texture(shadow_resolution, shadow_resolution, "rgba:i8"),
		new nix::DepthBuffer(shadow_resolution, shadow_resolution, "d24s8")});
	shadow_pass = new ShadowPassGL(this);

	ResourceManager::default_shader = "default.shader";
	if (config.get_str("renderer.shader-quality", "pbr") == "pbr") {
		ResourceManager::load_shader("module-lighting-pbr.shader");
		ResourceManager::load_shader("forward/module-surface-pbr.shader");
	} else {
		ResourceManager::load_shader("forward/module-surface.shader");
	}
	ResourceManager::load_shader("module-vertex-default.shader");
	ResourceManager::load_shader("module-vertex-animated.shader");
	ResourceManager::load_shader("module-vertex-instanced.shader");
	ResourceManager::load_shader("module-vertex-lines.shader");
	ResourceManager::load_shader("module-vertex-points.shader");
	ResourceManager::load_shader("module-geometry-points.shader");

	shader_fx = ResourceManager::load_shader("forward/3d-fx.shader");
}

void WorldRendererGLForward::prepare() {
	PerformanceMonitor::begin(channel);

	static int _frame = 0;
	_frame ++;
	if (_frame > 10) {
		if (world.ego)
			render_into_cubemap(depth_cube.get(), cube_map.get(), world.ego->pos);
		_frame = 0;
	}

	cam = cam_main;


	prepare_instanced_matrices();

	prepare_lights();

	if (shadow_index >= 0) {
		render_shadow_map(fb_shadow2.get(), 1);
		render_shadow_map(fb_shadow1.get(), 4);
	}

	PerformanceMonitor::end(channel);
}

void WorldRendererGLForward::draw() {
	PerformanceMonitor::begin(channel);

	auto fb = frame_buffer();
	bool flip_y = rendering_into_window();

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
	nix::set_wire(wireframe);


	background_renderer->draw();
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

	draw_world(true);
	Scheduler::handle_render_inject();
	break_point();
	PerformanceMonitor::end(ch_world);

	draw_particles(cam_main);
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

	draw_world(true);
	Scheduler::handle_render_inject();
	break_point();
	PerformanceMonitor::end(ch_world);

	draw_particles(cam);
	//nix::set_scissor(rect::EMPTY);

	nix::set_cull(nix::CullMode::DEFAULT);
}
#endif

void WorldRendererGLForward::draw_world(bool allow_material) {
	if (allow_material) {
		nix::bind_texture(3, fb_shadow1->depth_buffer.get());
		nix::bind_texture(4, fb_shadow2->depth_buffer.get());
		nix::bind_texture(5, cube_map.get());
	}

	draw_terrains(allow_material);
	draw_objects_instanced(allow_material);
	draw_objects_opaque(allow_material);
	draw_line_meshes(allow_material);
	draw_point_meshes(allow_material);
	if (allow_material)
		draw_objects_transparent(allow_material, type);
}

void WorldRendererGLForward::render_shadow_map(FrameBuffer *sfb, float scale) {
	nix::bind_frame_buffer(sfb);
	shadow_pass->set(shadow_proj, scale);
	shadow_pass->draw();
}


#endif
