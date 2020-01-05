/*
 * GBufferRenderer.cpp
 *
 *  Created on: 05.01.2020
 *      Author: michi
 */

#include "GBufferRenderer.h"

GBufferRenderer::GBufferRenderer() {
	width = 512;
	height = 512;
	tex_color = new vulkan::DynamicTexture(width, height, 1, "rgba:i8");
	tex_pos = new vulkan::DynamicTexture(width, height, 1, "rgba:f32");
	tex_normal = new vulkan::DynamicTexture(width, height, 1, "rgba:f32");

	VkExtent2D extent = {(unsigned)width, (unsigned)height};
	depth_buffer = new vulkan::DepthBuffer(extent, VK_FORMAT_D32_SFLOAT, true);

	default_render_pass = new vulkan::RenderPass({tex_color->format, tex_pos->format, tex_normal->format, depth_buffer->format}, true, false);
	frame_buffer = new vulkan::FrameBuffer(default_render_pass, {tex_color->view, tex_pos->view, tex_normal->view, depth_buffer->view}, extent);


	shader_into_gbuf = vulkan::Shader::load("3d-multi.shader");
	pipeline = vulkan::Pipeline::build(shader_into_gbuf, default_render_pass, 1, false);
	pipeline->set_dynamic({"viewport"});
	pipeline->create();
}

GBufferRenderer::~GBufferRenderer() {
	delete pipeline;
	delete shader_into_gbuf;
	delete frame_buffer;
	delete depth_buffer;
	delete tex_color;
	delete tex_pos;
	delete tex_normal;
}

bool GBufferRenderer::start_frame() {
	in_flight_fence->wait();
	return true;
}

void GBufferRenderer::end_frame() {
	vulkan::queue_submit_command_buffer(cb, {}, {}, in_flight_fence);
	vulkan::wait_device_idle();
}


vulkan::FrameBuffer *GBufferRenderer::current_frame_buffer() {
	return frame_buffer;
}


