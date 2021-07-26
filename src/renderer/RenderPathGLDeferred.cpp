/*
 * RenderPathGL.cpp
 *
 *  Created on: 07.08.2020
 *      Author: michi
 */

#include <GLFW/glfw3.h>

#include "RenderPathGLDeferred.h"
#include "RenderPathGL.h"
#include "../lib/nix/nix.h"
#include "../lib/file/msg.h"
#include "../lib/math/random.h"

#include "../helper/PerformanceMonitor.h"
#include "../helper/ResourceManager.h"
#include "../plugins/PluginManager.h"
#include "../gui/gui.h"
#include "../gui/Node.h"
#include "../gui/Picture.h"
#include "../gui/Text.h"
#include "../fx/Particle.h"
#include "../fx/Beam.h"
#include "../fx/ParticleManager.h"
#include "../world/Entity3D.h"
#include "../world/Camera.h"
#include "../world/Light.h"
#include "../world/Material.h"
#include "../world/Model.h"
#include "../world/Terrain.h"
#include "../world/World.h"
#include "../Config.h"
#include "../meta.h"


void break_point();

RenderPathGLDeferred::RenderPathGLDeferred(GLFWwindow* win, int w, int h, PerformanceMonitor *pm) : RenderPathGL(win, w, h, pm) {

	gbuffer = new nix::FrameBuffer({
		new nix::Texture(width, height, "rgba:f16"), // diffuse
		new nix::Texture(width, height, "rgba:f16"), // emission
		new nix::Texture(width, height, "rgba:f16"), // pos
		new nix::Texture(width, height, "rgba:f16"), // normal,reflection
		new nix::DepthBuffer(width, height, "d24s8")});

	fb_main = new nix::FrameBuffer({
		new nix::Texture(width, height, "rgba:f16"),
		new nix::DepthBuffer(width, height, "d24s8")});
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

	for (auto a: gbuffer->color_attachments)
		a->set_options("wrap=clamp,magfilter=nearest,minfilter=nearest");
	fb_main->color_attachments[0]->set_options("wrap=clamp");
	fb_small1->color_attachments[0]->set_options("wrap=clamp");
	fb_small2->color_attachments[0]->set_options("wrap=clamp");
	fb2->color_attachments[0]->set_options("wrap=clamp");
	fb3->color_attachments[0]->set_options("wrap=clamp");


	ResourceManager::default_shader = "default.shader";
	ResourceManager::load_shader("module-lighting-pbr.shader");
	ResourceManager::load_shader("deferred/module-surface.shader");
	ResourceManager::load_shader("module-vertex-default.shader");
	ResourceManager::load_shader("module-vertex-animated.shader");
	ResourceManager::load_shader("module-vertex-instanced.shader");

	shader_blur = ResourceManager::load_shader("forward/blur.shader");
	shader_depth = ResourceManager::load_shader("forward/depth.shader");
	shader_out = ResourceManager::load_shader("forward/hdr.shader");
	shader_gbuffer_out = ResourceManager::load_shader("deferred/out.shader");
	if (!shader_gbuffer_out->link_uniform_block("SSAO", 13))
		msg_error("SSAO");
	shader_fx = ResourceManager::load_shader("forward/3d-fx.shader");
	shader_2d = ResourceManager::load_shader("forward/2d.shader");

	ssao_sample_buffer = new nix::UniformBuffer();
	Array<vec4> ssao_samples;
	Random r;
	for (int i=0; i<64; i++) {
		auto v = r.dir() * pow(r.uniform01(), 1);
		ssao_samples.add(vec4(v.x, v.y, abs(v.z), 0));
	}
	ssao_sample_buffer->update_array(ssao_samples);
}

void RenderPathGLDeferred::draw() {
	perf_mon->tick(PMLabel::PRE);

	prepare_instanced_matrices();

	prepare_lights(cam);
	perf_mon->tick(PMLabel::PREPARE_LIGHTS);

	if (shadow_index >= 0) {
		render_shadow_map(fb_shadow.get(), 4);
		render_shadow_map(fb_shadow2.get(), 1);
	}
	perf_mon->tick(PMLabel::SHADOWS);

	render_into_texture(gbuffer.get(), cam, dynamic_fb_area());
	render_from_gbuffer(gbuffer.get(), fb_main.get());

	auto source = do_post_processing(fb_main.get());

	nix::bind_frame_buffer(nix::FrameBuffer::DEFAULT);

	render_out(source, fb_small2->color_attachments[0]);

	draw_gui(source);
}


void RenderPathGLDeferred::render_from_gbuffer(nix::FrameBuffer *source, nix::FrameBuffer *target) {
	auto s = shader_gbuffer_out.get();
	if (using_view_space)
		s->set_floats("eye_pos", &vector::ZERO.x, 3);
	else
		s->set_floats("eye_pos", &cam->get_owner<Entity3D>()->pos.x, 3);
	s->set_int("num_lights", lights.num);
	s->set_int("shadow_index", shadow_index);
	s->set_float("ambient_occlusion_radius", config.ambient_occlusion_radius);
	nix::bind_buffer(ssao_sample_buffer, 13);

	//auto mat_vp = matrix::scale(1,-1,1) * cam->m_projection * cam->m_view;
	//s->set_matrix("mat_vp", mat_vp);

	nix::bind_buffer(ubo_light, 1);
	auto tex = source->color_attachments;
	tex.add(source->depth_buffer);
	tex.add(fb_shadow->depth_buffer);
	tex.add(fb_shadow2->depth_buffer);
	process(tex, target, s);
}

void RenderPathGLDeferred::render_into_texture(nix::FrameBuffer *fb, Camera *cam, const rect &target_area) {
	nix::bind_frame_buffer(fb);
	nix::set_viewport(target_area);
	nix::set_scissor(target_area);

	float max_depth = cam->max_depth;
	cam->max_depth = 2000000;
	cam->update_matrices((float)fb->width / (float)fb->height);
	nix::set_projection_matrix(matrix::scale(1,-1,1) * cam->m_projection);

	nix::clear_color(world.background);
	nix::clear_z();

	draw_skyboxes(cam);
	perf_mon->tick(PMLabel::SKYBOXES);


	cam->max_depth = max_depth;
	cam->update_matrices((float)fb->width / (float)fb->height);
	nix::set_projection_matrix(matrix::scale(1,-1,1) * cam->m_projection);

	nix::bind_buffer(ubo_light, 1);
	nix::set_view_matrix(cam->m_view);
	nix::set_z(true, true);

	draw_world(true);
	plugin_manager.handle_render_inject();
	break_point();
	perf_mon->tick(PMLabel::WORLD);

	draw_particles();
	perf_mon->tick(PMLabel::PARTICLES);
}

void RenderPathGLDeferred::draw_world(bool allow_material) {
	draw_terrains(allow_material);
	draw_objects_instanced(allow_material);
	draw_objects_opaque(allow_material);
}

void RenderPathGLDeferred::render_shadow_map(nix::FrameBuffer *sfb, float scale) {
	nix::bind_frame_buffer(sfb);

	nix::set_projection_matrix(matrix::scale(scale, scale, 1) * shadow_proj);
	nix::set_view_matrix(matrix::ID);

	nix::clear_z();

	nix::set_z(true, true);

	draw_world(false);

	break_point();
}



