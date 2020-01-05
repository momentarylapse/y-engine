/*
 * GBufferRenderer.h
 *
 *  Created on: 05.01.2020
 *      Author: michi
 */

#ifndef SRC_RENDERER_GBUFFERRENDERER_H_
#define SRC_RENDERER_GBUFFERRENDERER_H_


#include "Renderer.h"


class GBufferRenderer : public Renderer {
public:
	GBufferRenderer();
	~GBufferRenderer() override;

	vulkan::Texture *tex_color;
	vulkan::Texture *tex_pos;
	vulkan::Texture *tex_normal;

	vulkan::DepthBuffer *depth_buffer;
	vulkan::FrameBuffer *frame_buffer;

	vulkan::Shader *shader_into_gbuf;
	vulkan::Pipeline *pipeline;

	bool start_frame() override;
	void end_frame() override;

	vulkan::FrameBuffer *current_frame_buffer() override;
};


#endif /* SRC_RENDERER_GBUFFERRENDERER_H_ */
