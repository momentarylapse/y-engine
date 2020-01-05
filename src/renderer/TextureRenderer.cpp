/*
 * TextureRenderer.cpp
 *
 *  Created on: 05.01.2020
 *      Author: michi
 */


#include "TextureRenderer.h"



TextureRenderer::TextureRenderer(vulkan::Texture *t) {
	tex = t;
	width = tex->width;
	height = tex->height;

	VkExtent2D extent = {(unsigned)width, (unsigned)height};
	depth_buffer = new vulkan::DepthBuffer(extent, VK_FORMAT_D32_SFLOAT, true);

	default_render_pass = new vulkan::RenderPass({tex->format, depth_buffer->format}, true, false);
	frame_buffer = new vulkan::FrameBuffer(default_render_pass, {tex->view, depth_buffer->view}, extent);
}

TextureRenderer::~TextureRenderer() {
	delete depth_buffer;
	delete frame_buffer;
}

bool TextureRenderer::start_frame() {
	in_flight_fence->wait();
	return true;
}

void TextureRenderer::end_frame() {
	vulkan::queue_submit_command_buffer(cb, {}, {}, in_flight_fence);
	vulkan::wait_device_idle();
}


vulkan::FrameBuffer *TextureRenderer::current_frame_buffer() {
	return frame_buffer;
}



