/*
 * ShadowMapRenderer.cpp
 *
 *  Created on: 06.01.2020
 *      Author: michi
 */

#include "ShadowMapRenderer.h"

#if HAS_LIB_VULKAN


ShadowMapRenderer::ShadowMapRenderer(const string &shader_filename) {
	width = 512;
	height = 512;

	depth_buffer = new vulkan::DepthBuffer(width, height, "d:f32", true);

	_default_render_pass = new vulkan::RenderPass({depth_buffer->format}, "clear");
	frame_buffer = new vulkan::FrameBuffer(width, height, _default_render_pass, {depth_buffer->view});


	shader = vulkan::Shader::load(shader_filename);
	pipeline = new vulkan::Pipeline(shader, _default_render_pass, 0, 1);

}
ShadowMapRenderer::~ShadowMapRenderer() {
	delete _default_render_pass;
	delete pipeline;
	delete shader;
	delete frame_buffer;
	delete depth_buffer;
}

bool ShadowMapRenderer::start_frame() {
	//in_flight_fence->wait();
	cb->begin();
	return true;

}
void ShadowMapRenderer::end_frame() {
	//cb->barrier({tex_color, tex_emission, tex_normal, tex_pos, depth_buffer}, 0);
	cb->barrier({depth_buffer}, 0);
	cb->end();
	vulkan::queue_submit_command_buffer(cb, {}, {}, in_flight_fence);
	in_flight_fence->wait();
}

#endif

