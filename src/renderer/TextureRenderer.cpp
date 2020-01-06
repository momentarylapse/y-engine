/*
 * TextureRenderer.cpp
 *
 *  Created on: 05.01.2020
 *      Author: michi
 */


#include "TextureRenderer.h"

namespace vulkan {
	extern VkCompareOp next_compare_op;
}


TextureRenderer::TextureRenderer(vulkan::Texture *t) {
	tex = t;
	width = tex->width;
	height = tex->height;

	//vulkan::next_compare_op = VK_COMPARE_OP_LESS;
	depth_buffer = new vulkan::DepthBuffer(width, height, "d:f32", true);
	//vulkan::next_compare_op = VK_COMPARE_OP_ALWAYS;

	_default_render_pass = new vulkan::RenderPass({tex->format, depth_buffer->format}, true, false);
	frame_buffer = new vulkan::FrameBuffer(width, height, _default_render_pass, {tex->view, depth_buffer->view});
}

TextureRenderer::~TextureRenderer() {
	delete _default_render_pass;
	delete depth_buffer;
	delete frame_buffer;
}

bool TextureRenderer::start_frame() {
	cb->begin();
	return true;
}

void TextureRenderer::end_frame() {
	//cb->barrier({tex, depth_buffer}, 0);
	cb->barrier({depth_buffer}, 0);
	cb->end();

	vulkan::queue_submit_command_buffer(cb, {}, {}, in_flight_fence);
	in_flight_fence->wait();
	vulkan::wait_device_idle();
}


vulkan::FrameBuffer *TextureRenderer::current_frame_buffer() {
	return frame_buffer;
}



