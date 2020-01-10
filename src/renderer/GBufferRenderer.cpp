/*
 * GBufferRenderer.cpp
 *
 *  Created on: 05.01.2020
 *      Author: michi
 */

#include "GBufferRenderer.h"

GBufferRenderer::GBufferRenderer(int w, int h) {
	shader_into_gbuf = vulkan::Shader::load("3d-multi.shader");

	_create(w, h);
}

GBufferRenderer::~GBufferRenderer() {
	_destroy();
	delete shader_into_gbuf;
}

void GBufferRenderer::_create(int w, int h) {
	width = w;
	height = h;
	tex_color = new vulkan::DynamicTexture(width, height, 1, "rgba:i8");
	tex_emission = new vulkan::DynamicTexture(width, height, 1, "rgba:i8");
	tex_pos = new vulkan::DynamicTexture(width, height, 1, "rgba:f32");
	tex_normal = new vulkan::DynamicTexture(width, height, 1, "rgba:f32");

	depth_buffer = new vulkan::DepthBuffer(width, height, "d:f32", true);

	_default_render_pass = new vulkan::RenderPass({tex_color->format, tex_emission->format, tex_pos->format, tex_normal->format, depth_buffer->format}, true, false);
	g_buffer = new vulkan::FrameBuffer(width, height, _default_render_pass, {tex_color->view, tex_emission->view, tex_pos->view, tex_normal->view, depth_buffer->view});


	pipeline_into_gbuf = new vulkan::Pipeline(shader_into_gbuf, _default_render_pass, 1);
}

void GBufferRenderer::_destroy() {
	delete _default_render_pass;
	delete pipeline_into_gbuf;
	delete g_buffer;
	delete depth_buffer;
	delete tex_color;
	delete tex_pos;
	delete tex_normal;
}

void GBufferRenderer::resize(int w, int h) {
	_destroy();
	_create(w, h);
}

bool GBufferRenderer::start_frame() {
	cb->begin();
	return true;
}

void GBufferRenderer::end_frame() {
	//cb->barrier({tex_color, tex_emission, tex_normal, tex_pos, depth_buffer}, 0);
	cb->barrier({depth_buffer}, 0);
	cb->end();

	vulkan::queue_submit_command_buffer(cb, {}, {}, in_flight_fence);
	in_flight_fence->wait();
	vulkan::wait_device_idle();
}


vulkan::FrameBuffer *GBufferRenderer::current_frame_buffer() {
	return g_buffer;
}


