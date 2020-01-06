/*
 * GBufferRenderer.cpp
 *
 *  Created on: 05.01.2020
 *      Author: michi
 */

#include "GBufferRenderer.h"

GBufferRenderer::GBufferRenderer() {
	width = 1024;
	height = 768;
	tex_color = new vulkan::DynamicTexture(width, height, 1, "rgba:i8");
	tex_emission = new vulkan::DynamicTexture(width, height, 1, "rgba:i8");
	tex_pos = new vulkan::DynamicTexture(width, height, 1, "rgba:f32");
	tex_normal = new vulkan::DynamicTexture(width, height, 1, "rgba:f32");

	depth_buffer = new vulkan::DepthBuffer(width, height, "d:f32", true);

	render_pass_into_g = new vulkan::RenderPass({tex_color->format, tex_emission->format, tex_pos->format, tex_normal->format, depth_buffer->format}, true, false);
	g_buffer = new vulkan::FrameBuffer(width, height, render_pass_into_g, {tex_color->view, tex_emission->view, tex_pos->view, tex_normal->view, depth_buffer->view});


	shader_into_gbuf = vulkan::Shader::load("3d-multi.shader");
	pipeline_into_gbuf = new vulkan::Pipeline(shader_into_gbuf, render_pass_into_g, 1);


	tex_output = new vulkan::DynamicTexture(width, height, 1, "rgba:i8");

	render_pass_merge = new vulkan::RenderPass({tex_output->format, depth_buffer->format}, true, false);
	frame_buffer = new vulkan::FrameBuffer(width, height, render_pass_merge, {tex_output->view, depth_buffer->view});


	shader_merge_base = vulkan::Shader::load("2d-gbuf-emission.shader");
	shader_merge_light = vulkan::Shader::load("2d-gbuf-light.shader");
	shader_merge_light_shadow = vulkan::Shader::load("2d-gbuf-light-shadow.shader");
	shader_merge_fog = vulkan::Shader::load("2d-gbuf-fog.shader");
	pipeline_merge = new vulkan::Pipeline(shader_merge_base, render_pass_merge, 1);

	_cfb = nullptr;
}

GBufferRenderer::~GBufferRenderer() {
	delete pipeline_merge;
	delete frame_buffer;
	delete shader_merge_base;
	delete shader_merge_light;
	delete shader_merge_light_shadow;
	delete shader_merge_fog;
	delete render_pass_merge;
	delete tex_output;

	delete render_pass_into_g;
	delete pipeline_into_gbuf;
	delete shader_into_gbuf;
	delete g_buffer;
	delete depth_buffer;
	delete tex_color;
	delete tex_pos;
	delete tex_normal;
}

bool GBufferRenderer::start_frame_into_gbuf() {
	in_flight_fence->wait();
	_cfb = g_buffer;
	cb->begin();
	return true;
}

void GBufferRenderer::end_frame_into_gbuf() {
	//cb->barrier({tex_color, tex_emission, tex_normal, tex_pos, depth_buffer}, 0);
	cb->barrier({depth_buffer}, 0);
	cb->end();
	vulkan::queue_submit_command_buffer(cb, {}, {}, in_flight_fence);
	vulkan::wait_device_idle();
}

bool GBufferRenderer::start_frame_merge() {
	in_flight_fence->wait();
	_cfb = frame_buffer;
	return true;
}

void GBufferRenderer::end_frame_merge() {
	vulkan::queue_submit_command_buffer(cb, {}, {}, in_flight_fence);
	vulkan::wait_device_idle();
}


vulkan::FrameBuffer *GBufferRenderer::current_frame_buffer() {
	return _cfb;
}


