/*
 * HDRRendererGL.cpp
 *
 *  Created on: 23 Nov 2021
 *      Author: michi
 */

#include "HDRRendererGL.h"
#ifdef USING_OPENGL
#include "../base.h"
#include "../../lib/nix/nix.h"
#include "../../lib/math/vec2.h"
#include "../../lib/math/rect.h"
#include "../../lib/file/msg.h"
#include "../../helper/PerformanceMonitor.h"
#include "../../helper/ResourceManager.h"
#include "../../Config.h"
#include "../../world/Camera.h"

static float resolution_scale_x = 1.0f;
static float resolution_scale_y = 1.0f;


HDRRendererGL::HDRRendererGL(Renderer *parent) : Renderer("hdr", parent) {
	ch_post_blur = PerformanceMonitor::create_channel("blur", channel);
	ch_out = PerformanceMonitor::create_channel("out", channel);


	depth_buffer = new nix::DepthBuffer(width, height, "d24s8");
	if (config.antialiasing_method == AntialiasingMethod::MSAA) {
		msg_error("yes msaa");
		fb_main = new nix::FrameBuffer({
			new nix::TextureMultiSample(width, height, 4, "rgba:f16"),
			//depth_buffer});
			new nix::RenderBuffer(width, height, 4, "d24s8")});
	} else {
		msg_error("no msaa");
		fb_main = new nix::FrameBuffer({
			new nix::Texture(width, height, "rgba:f16"),
			depth_buffer});
			//new nix::RenderBuffer(width, height, "d24s8)});
	}
	fb_small1 = new nix::FrameBuffer({
		new nix::Texture(width/2, height/2, "rgba:f16")});
	fb_small2 = new nix::FrameBuffer({
		new nix::Texture(width/2, height/2, "rgba:f16")});

	if (fb_main->color_attachments[0]->type != nix::Texture::Type::MULTISAMPLE)
		fb_main->color_attachments[0]->set_options("wrap=clamp");
	fb_small1->color_attachments[0]->set_options("wrap=clamp");
	fb_small2->color_attachments[0]->set_options("wrap=clamp");

	shader_blur = ResourceManager::load_shader("forward/blur.shader");
	shader_out = ResourceManager::load_shader("forward/hdr.shader");

	vb_2d = new nix::VertexBuffer("3f,3f,2f|i");
	vb_2d->create_rect(rect(-1,1, -1,1));
}

HDRRendererGL::~HDRRendererGL() {
}

void HDRRendererGL::prepare() {
	if (child)
		child->prepare();

	nix::bind_frame_buffer(fb_main.get());

	if (child)
		child->draw();


	PerformanceMonitor::begin(ch_post_blur);
	process_blur(fb_main.get(), fb_small1.get(), 1.0f, {2,0});
	process_blur(fb_small1.get(), fb_small2.get(), 0.0f, {0,1});
	break_point();
	PerformanceMonitor::end(ch_post_blur);
}

void HDRRendererGL::draw() {
	render_out(fb_main.get(), fb_small2->color_attachments[0].get());
}

void HDRRendererGL::process_blur(FrameBuffer *source, FrameBuffer *target, float threshold, const vec2 &axis) {
	float r = cam->bloom_radius * resolution_scale_x;
	shader_blur->set_float("radius", r);
	shader_blur->set_float("threshold", threshold / cam->exposure);
	shader_blur->set_floats("axis", &axis.x, 2);
	process(weak(source->color_attachments), target, shader_blur.get());
}

void HDRRendererGL::process(const Array<Texture*> &source, FrameBuffer *target, Shader *shader) {
	nix::bind_frame_buffer(target);
	nix::set_scissor(rect(0, target->width*resolution_scale_x, 0, target->height*resolution_scale_y));
	nix::set_z(false, false);
	//nix::set_projection_ortho_relative();
	//nix::set_view_matrix(matrix::ID);
	//nix::set_model_matrix(matrix::ID);
	shader->set_floats("resolution_scale", &resolution_scale_x, 2);
	nix::set_shader(shader);

	nix::set_textures(source);
	nix::draw_triangles(vb_2d);
	nix::set_scissor(rect::EMPTY);
}

void HDRRendererGL::render_out(FrameBuffer *source, Texture *bloom) {
	PerformanceMonitor::begin(ch_out);

	nix::set_textures({source->color_attachments[0].get(), bloom});
	nix::set_shader(shader_out.get());
	shader_out->set_float("exposure", cam->exposure);
	shader_out->set_float("bloom_factor", cam->bloom_factor);
	shader_out->set_float("scale_x", resolution_scale_x);
	shader_out->set_float("scale_y", resolution_scale_y);
	nix::set_projection_matrix(matrix::ID);
	nix::set_view_matrix(matrix::ID);
	nix::set_model_matrix(matrix::ID);

	nix::set_z(false, false);

	nix::draw_triangles(vb_2d);

	break_point();
	PerformanceMonitor::end(ch_out);
}

#endif
