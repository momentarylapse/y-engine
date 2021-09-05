/*
 * RenderPathGLForward.cpp
 *
 *  Created on: Jun 2, 2021
 *      Author: michi
 */

#include <GLFW/glfw3.h>
#include "RenderPathGLForward.h"
#include "../lib/nix/nix.h"
#include "../lib/image/image.h"
#include "../lib/file/msg.h"

#include "../helper/PerformanceMonitor.h"
#include "../helper/ResourceManager.h"
#include "../helper/Scheduler.h"
#include "../plugins/PluginManager.h"
#include "../gui/gui.h"
#include "../gui/Node.h"
#include "../gui/Picture.h"
#include "../gui/Text.h"
#include "../fx/Particle.h"
#include "../fx/Beam.h"
#include "../fx/ParticleManager.h"
#include "../world/Camera.h"
#include "../world/Light.h"
#include "../world/Entity3D.h"
#include "../world/Material.h"
#include "../world/Model.h"
#include "../world/Object.h" // meh
#include "../world/Terrain.h"
#include "../world/World.h"
#include "../Config.h"
#include "../meta.h"

// https://learnopengl.com/Advanced-OpenGL/Anti-Aliasing

namespace nix {
	int total_mem();
	int available_mem();
}
matrix jitter(float w, float h, int uid);
void break_point();


RenderPathGLForward::RenderPathGLForward(GLFWwindow* win, int w, int h) : RenderPathGL(win, w, h, RenderPathType::FORWARD) {

	depth_buffer = new nix::DepthBuffer(width, height, "d24s8");
	if (config.antialiasing_method == AntialiasingMethod::MSAA) {
		fb_main = new nix::FrameBuffer({
			new nix::TextureMultiSample(width, height, 4, "rgba:f16"),
			//depth_buffer});
			new nix::RenderBuffer(width, height, 4, "d24s8")});
	} else {
		fb_main = new nix::FrameBuffer({
			new nix::Texture(width, height, "rgba:f16"),
			depth_buffer});
			//new nix::RenderBuffer(width, height, "d24s8)});
	}
	fb_small1 = new nix::FrameBuffer({
		new nix::Texture(width/2, height/2, "rgba:f16")});
	fb_small2 = new nix::FrameBuffer({
		new nix::Texture(width/2, height/2, "rgba:f16")});
	fb2 = new nix::FrameBuffer({
		new nix::Texture(width, height, "rgba:f16")});
	fb3 = new nix::FrameBuffer({
		new nix::Texture(width, height, "rgba:f16")});
	fb_shadow = new nix::FrameBuffer({
		new nix::DepthBuffer(shadow_resolution, shadow_resolution, "d24s8")});
	fb_shadow2 = new nix::FrameBuffer({
		new nix::DepthBuffer(shadow_resolution, shadow_resolution, "d24s8")});

	if (fb_main->color_attachments[0]->type != nix::Texture::Type::MULTISAMPLE)
		fb_main->color_attachments[0]->set_options("wrap=clamp");
	fb_small1->color_attachments[0]->set_options("wrap=clamp");
	fb_small2->color_attachments[0]->set_options("wrap=clamp");
	fb2->color_attachments[0]->set_options("wrap=clamp");
	fb3->color_attachments[0]->set_options("wrap=clamp");

	ResourceManager::default_shader = "default.shader";
	if (config.get_str("renderer.shader-quality", "") == "pbr") {
		ResourceManager::load_shader("module-lighting-pbr.shader");
		ResourceManager::load_shader("forward/module-surface-pbr.shader");
	} else {
		ResourceManager::load_shader("forward/module-surface.shader");
	}
	ResourceManager::load_shader("module-vertex-default.shader");
	ResourceManager::load_shader("module-vertex-animated.shader");
	ResourceManager::load_shader("module-vertex-instanced.shader");

	shader_blur = ResourceManager::load_shader("forward/blur.shader");
	shader_depth = ResourceManager::load_shader("forward/depth.shader");
	shader_out = ResourceManager::load_shader("forward/hdr.shader");
	shader_fx = ResourceManager::load_shader("forward/3d-fx.shader");
	shader_2d = ResourceManager::load_shader("forward/2d.shader");
	shader_resolve_multisample = ResourceManager::load_shader("forward/resolve-multisample.shader");


	if (nix::total_mem() > 0) {
		msg_write(format("VRAM: %d mb  of  %d mb available", nix::available_mem() / 1024, nix::total_mem() / 1024));
	}
}

void RenderPathGLForward::draw() {
	PerformanceMonitor::begin(ch_render);

	static int _frame = 0;
	_frame ++;
	if (_frame > 10) {
		if (world.ego)
			render_into_cubemap(depth_cube.get(), cube_map.get(), world.ego->pos);
		_frame = 0;
	}


	prepare_instanced_matrices();

	prepare_lights(cam);

	PerformanceMonitor::begin(ch_shadow);
	if (shadow_index >= 0) {
		render_shadow_map(fb_shadow.get(), 4);
		render_shadow_map(fb_shadow2.get(), 1);
	}
	PerformanceMonitor::end(ch_shadow);

	render_into_texture(fb_main.get(), cam, dynamic_fb_area());

	auto source = fb_main.get();
	if (config.antialiasing_method == AntialiasingMethod::MSAA)
		source = resolve_multisampling(source);

	source = do_post_processing(source);


	nix::bind_frame_buffer(nix::FrameBuffer::DEFAULT);
	render_out(source, fb_small2->color_attachments[0].get());

	draw_gui(source);
	PerformanceMonitor::end(ch_render);
}

void RenderPathGLForward::render_into_texture(nix::FrameBuffer *fb, Camera *cam, const rect &target_area) {
	PerformanceMonitor::begin(ch_bg);
	nix::bind_frame_buffer(fb);
	nix::set_viewport(target_area);
	nix::set_scissor(target_area);

	auto m = matrix::scale(1,-1,1);
	if (config.antialiasing_method == AntialiasingMethod::TAA)
		 m *= jitter(fb->width, fb->height, 0);

	// skyboxes
	float max_depth = cam->max_depth;
	cam->max_depth = 2000000;
	cam->update_matrices((float)fb->width / (float)fb->height);
	nix::set_projection_matrix(m * cam->m_projection);

	nix::clear_color(world.background);
	nix::clear_z();

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

	draw_world(true);
	Scheduler::handle_render_inject();
	break_point();
	PerformanceMonitor::end(ch_world);

	draw_particles();
	nix::set_scissor(rect::EMPTY);
}

void RenderPathGLForward::draw_world(bool allow_material) {
	draw_terrains(allow_material);
	draw_objects_instanced(allow_material);
	draw_objects_opaque(allow_material);
	if (allow_material)
		draw_objects_transparent(allow_material, type);
}

void RenderPathGLForward::render_shadow_map(nix::FrameBuffer *sfb, float scale) {
	nix::bind_frame_buffer(sfb);

	auto m = matrix::scale(scale, scale, 1);
	//m = m * jitter(sfb->width*8, sfb->height*8, 1);
	nix::set_projection_matrix(m * shadow_proj);
	nix::set_view_matrix(matrix::ID);
	nix::set_model_matrix(matrix::ID);

	nix::clear_z();

	nix::set_z(true, true);
	//nix::set_shader(shader_shadow.get());


	draw_world(false);

	break_point();
}



