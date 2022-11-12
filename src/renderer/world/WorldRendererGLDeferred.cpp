/*
 * WorldRendererGL.cpp
 *
 *  Created on: 07.08.2020
 *      Author: michi
 */

#include "WorldRendererGLDeferred.h"

#include <GLFW/glfw3.h>

#ifdef USING_OPENGL
#include "../base.h"
#include "../../lib/nix/nix.h"
#include "../../lib/os/msg.h"
#include "../../lib/math/random.h"
#include "../../lib/math/vec4.h"

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
#include "../../graphics-impl.h"


WorldRendererGLDeferred::WorldRendererGLDeferred(Renderer *parent) : WorldRendererGL("world/def", parent, RenderPathType::DEFERRED) {

	gbuffer = new nix::FrameBuffer({
		new nix::Texture(width, height, "rgba:f16"), // diffuse
		new nix::Texture(width, height, "rgba:f16"), // emission
		new nix::Texture(width, height, "rgba:f16"), // pos
		new nix::Texture(width, height, "rgba:f16"), // normal,reflection
		new nix::DepthBuffer(width, height, "d24s8")});

	fb_shadow1 = new nix::FrameBuffer({
		new nix::DepthBuffer(shadow_resolution, shadow_resolution, "d24s8")});
	fb_shadow2 = new nix::FrameBuffer({
		new nix::DepthBuffer(shadow_resolution, shadow_resolution, "d24s8")});

	for (auto a: gbuffer->color_attachments)
		a->set_options("wrap=clamp,magfilter=nearest,minfilter=nearest");


	ResourceManager::default_shader = "default.shader";
	ResourceManager::load_shader("module-lighting-pbr.shader");
	ResourceManager::load_shader("deferred/module-surface.shader");
	ResourceManager::load_shader("forward/module-surface-pbr.shader");
	ResourceManager::load_shader("module-vertex-default.shader");
	ResourceManager::load_shader("module-vertex-animated.shader");
	ResourceManager::load_shader("module-vertex-instanced.shader");

	shader_gbuffer_out = ResourceManager::load_shader("deferred/out.shader");
	if (!shader_gbuffer_out->link_uniform_block("SSAO", 13))
		msg_error("SSAO");
	shader_fx = ResourceManager::load_shader("forward/3d-fx.shader");

	ssao_sample_buffer = new nix::UniformBuffer();
	Array<vec4> ssao_samples;
	Random r;
	for (int i=0; i<64; i++) {
		auto v = r.dir() * pow(r.uniform01(), 1);
		ssao_samples.add(vec4(v.x, v.y, abs(v.z), 0));
	}
	ssao_sample_buffer->update_array(ssao_samples);

	ch_gbuf_out = PerformanceMonitor::create_channel("gbuf-out", channel);
	ch_trans = PerformanceMonitor::create_channel("trans", channel);
}

void WorldRendererGLDeferred::prepare() {
	PerformanceMonitor::begin(channel);
	prepare_instanced_matrices();

	prepare_lights(cam_main);

	PerformanceMonitor::begin(ch_shadow);
	if (shadow_index >= 0) {
		render_shadow_map(fb_shadow1.get(), 4);
		render_shadow_map(fb_shadow2.get(), 1);
	}
	PerformanceMonitor::end(ch_shadow);

	render_into_gbuffer(gbuffer.get(), cam_main);

	//auto source = do_post_processing(fb_main.get());

	PerformanceMonitor::end(channel);
}

void WorldRendererGLDeferred::draw() {
	PerformanceMonitor::begin(channel);

	auto target = parent->frame_buffer();

	draw_background(target, cam_main);

	render_out_from_gbuffer(gbuffer.get());

	PerformanceMonitor::begin(ch_trans);
	bool flip_y = rendering_into_window();
	mat4 m = flip_y ? mat4::scale(1,-1,1) : mat4::ID;
	cam_main->update_matrices((float)target->width / (float)target->height);
	nix::set_projection_matrix(m * cam_main->m_projection);
	nix::bind_buffer(1, ubo_light);
	nix::set_view_matrix(cam_main->view_matrix());
	nix::set_z(true, true);

	draw_objects_transparent(true, RenderPathType::FORWARD);
	draw_particles(cam_main);

	nix::set_z(false, false);
	nix::set_projection_matrix(mat4::ID);
	nix::set_view_matrix(mat4::ID);
	PerformanceMonitor::end(ch_trans);

	PerformanceMonitor::end(channel);
}

void WorldRendererGLDeferred::draw_background(nix::FrameBuffer *fb, Camera *cam) {
	PerformanceMonitor::begin(ch_bg);

	float max_depth = cam->max_depth;
	cam->max_depth = 2000000;
	bool flip_y = rendering_into_window();
	mat4 m = flip_y ? mat4::scale(1,-1,1) : mat4::ID;
	cam->update_matrices((float)fb->width / (float)fb->height);
	nix::set_projection_matrix(m * cam->m_projection);

	//nix::clear_color(Green);
	nix::clear_color(world.background);

	draw_skyboxes(cam);
	PerformanceMonitor::end(ch_bg);

}

void WorldRendererGLDeferred::render_out_from_gbuffer(nix::FrameBuffer *source) {
	PerformanceMonitor::begin(ch_gbuf_out);
	auto s = shader_gbuffer_out.get();
	if (using_view_space)
		s->set_floats("eye_pos", &vec3::ZERO.x, 3);
	else
		s->set_floats("eye_pos", &cam_main->owner->pos.x, 3); // NAH
	s->set_int("num_lights", lights.num);
	s->set_int("shadow_index", shadow_index);
	s->set_float("ambient_occlusion_radius", config.ambient_occlusion_radius);
	nix::bind_buffer(13, ssao_sample_buffer);

	nix::bind_buffer(1, ubo_light);
	auto tex = weak(source->color_attachments);
	tex.add(source->depth_buffer.get());
	tex.add(fb_shadow1->depth_buffer.get());
	tex.add(fb_shadow2->depth_buffer.get());
	nix::set_textures(tex);


	nix::set_z(false, false);
	float resolution_scale_x = 1.0f;
	s->set_floats("resolution_scale", &resolution_scale_x, 2);
	nix::set_shader(s);

	nix::vb_temp->create_quad(rect::ID_SYM, dynamicly_scaled_source());
	nix::draw_triangles(nix::vb_temp);


	break_point();
	PerformanceMonitor::end(ch_gbuf_out);
}

void WorldRendererGLDeferred::render_into_texture(nix::FrameBuffer *fb, Camera *cam) {}

void WorldRendererGLDeferred::render_into_gbuffer(nix::FrameBuffer *fb, Camera *cam) {
	PerformanceMonitor::begin(ch_world);
	nix::bind_frame_buffer(fb);
	nix::set_viewport(dynamicly_scaled_area(fb));

	float max_depth = cam->max_depth;
	cam->max_depth = 2000000;
	cam->update_matrices((float)fb->width / (float)fb->height);
	nix::set_projection_matrix(mat4::scale(1,-1,1) * cam->m_projection);

	//nix::clear_color(Green);//world.background);
	nix::clear_z();
	//fb->clear_color(2, color(0, 0,0,max_depth * 0.99f));
	fb->clear_color(0, color(-1, 0,1,0));


	cam->max_depth = max_depth;
	cam->update_matrices((float)fb->width / (float)fb->height);
	nix::set_projection_matrix(mat4::scale(1,1,1) * cam->m_projection);

	nix::bind_buffer(1, ubo_light);
	nix::set_view_matrix(cam->view_matrix());
	nix::set_z(true, true);
	nix::set_cull(nix::CullMode::CW);

	draw_world(true);
	Scheduler::handle_render_inject();
	break_point();
	PerformanceMonitor::end(ch_world);

	nix::set_cull(nix::CullMode::DEFAULT);
	draw_particles(cam);
}

void WorldRendererGLDeferred::draw_world(bool allow_material) {
	draw_terrains(allow_material);
	draw_objects_instanced(allow_material);
	draw_objects_opaque(allow_material);
	draw_line_meshes(allow_material);
	draw_point_meshes(allow_material);
}

void WorldRendererGLDeferred::render_shadow_map(nix::FrameBuffer *sfb, float scale) {
	nix::bind_frame_buffer(sfb);

	nix::set_projection_matrix(mat4::translation(vec3(0,0,0.5f)) * mat4::scale(1,1,0.5f) * mat4::scale(scale, scale, 1) * shadow_proj);
	nix::set_view_matrix(mat4::ID);

	nix::clear_z();

	nix::set_z(true, true);

	draw_world(false);

	break_point();
}


#endif
