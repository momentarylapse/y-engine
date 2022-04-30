/*
 * WorldRendererGLForward.cpp
 *
 *  Created on: Jun 2, 2021
 *      Author: michi
 */

#include "WorldRendererGLForward.h"

#include <GLFW/glfw3.h>
#ifdef USING_OPENGL
#include "../base.h"
#include "../helper/jitter.h"
#include "../../lib/nix/nix.h"
#include "../../lib/image/image.h"
#include "../../lib/file/msg.h"

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


WorldRendererGLForward::WorldRendererGLForward(Renderer *parent) : WorldRendererGL("world/fw", parent, RenderPathType::FORWARD) {

	fb_shadow1 = new nix::FrameBuffer({
		new nix::Texture(shadow_resolution, shadow_resolution, "rgba:i8"),
		new nix::DepthBuffer(shadow_resolution, shadow_resolution, "d24s8")});
	fb_shadow2 = new nix::FrameBuffer({
		new nix::Texture(shadow_resolution, shadow_resolution, "rgba:i8"),
		new nix::DepthBuffer(shadow_resolution, shadow_resolution, "d24s8")});

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


	prepare_instanced_matrices();

	prepare_lights(cam_main);

	PerformanceMonitor::begin(ch_shadow);
	if (shadow_index >= 0) {
		render_shadow_map(fb_shadow2.get(), 1);
		render_shadow_map(fb_shadow1.get(), 4);
	}
	PerformanceMonitor::end(ch_shadow);

	/*render_into_texture(fb_main.get(), cam, dynamic_fb_area());

	auto source = fb_main.get();
	if (config.antialiasing_method == AntialiasingMethod::MSAA)
		source = resolve_multisampling(source);

	source = do_post_processing(source);


	nix::bind_frame_buffer(parent->current_frame_buffer());
	render_out(source, fb_small2->color_attachments[0].get());*/

	PerformanceMonitor::end(channel);
}

void WorldRendererGLForward::draw() {
	PerformanceMonitor::begin(channel);

	auto fb = frame_buffer();
	bool flip_y = rendering_into_window();

	PerformanceMonitor::begin(ch_bg);

	auto m = flip_y ? matrix::scale(1,-1,1) : matrix::ID;
	if (config.antialiasing_method == AntialiasingMethod::TAA)
		 m *= jitter(fb->width, fb->height, 0);

	// skyboxes
	float max_depth = cam_main->max_depth;
	cam_main->max_depth = 2000000;
	cam_main->update_matrices((float)fb->width / (float)fb->height);
	nix::set_projection_matrix(m * cam_main->m_projection);

	nix::clear_color(world.background);
	nix::clear_z();
	nix::set_cull(flip_y ? nix::CullMode::CCW : nix::CullMode::CW);
	nix::set_wire(wireframe);

	draw_skyboxes(cam_main);
	PerformanceMonitor::end(ch_bg);


	// world
	PerformanceMonitor::begin(ch_world);
	cam_main->max_depth = max_depth;
	cam_main->update_matrices((float)fb->width / (float)fb->height);
	nix::set_projection_matrix(m * cam_main->m_projection);

	nix::bind_buffer(ubo_light, 1);
	nix::set_view_matrix(cam_main->view_matrix());
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

void WorldRendererGLForward::render_into_texture(FrameBuffer *fb, Camera *cam) {
	//draw();


	prepare_lights(cam);

	PerformanceMonitor::begin(ch_shadow);
	if (shadow_index >= 0) {
		render_shadow_map(fb_shadow1.get(), 4);
		render_shadow_map(fb_shadow2.get(), 1);
	}
	PerformanceMonitor::end(ch_shadow);

	bool flip_y = false;
	nix::bind_frame_buffer(fb);

	PerformanceMonitor::begin(ch_bg);

	auto m = flip_y ? matrix::scale(1,-1,1) : matrix::ID;
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

	nix::bind_buffer(ubo_light, 1);
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

void WorldRendererGLForward::draw_world(bool allow_material) {
	draw_terrains(allow_material);
	draw_objects_instanced(allow_material);
	draw_objects_opaque(allow_material);
	if (allow_material)
		draw_objects_transparent(allow_material, type);
}

void WorldRendererGLForward::render_shadow_map(FrameBuffer *sfb, float scale) {
	nix::bind_frame_buffer(sfb);

	auto m = matrix::scale(scale, scale, 1);
	//m = m * jitter(sfb->width*8, sfb->height*8, 1);
	nix::set_projection_matrix(m * shadow_proj);
	nix::set_view_matrix(matrix::ID);
	nix::set_model_matrix(matrix::ID);

	nix::clear_z();

	nix::set_z(true, true);


	draw_world(false);

	break_point();
}


#endif
