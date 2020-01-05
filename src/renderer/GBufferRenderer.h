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

	vulkan::Texture *tex_emission;
	vulkan::Texture *tex_color;
	vulkan::Texture *tex_pos;
	vulkan::Texture *tex_normal;

	vulkan::DepthBuffer *depth_buffer;
	vulkan::FrameBuffer *g_buffer;

	vulkan::Shader *shader_into_gbuf;
	vulkan::Pipeline *pipeline_into_gbuf;
	vulkan::RenderPass *render_pass_into_g;

	bool start_frame_into_gbuf();
	void end_frame_into_gbuf();


	vulkan::Texture *tex_output;

	//vulkan::DepthBuffer *depth_buffer;
	vulkan::FrameBuffer *frame_buffer;

	vulkan::Shader *shader_merge_base;
	vulkan::Shader *shader_merge_light;
	vulkan::Pipeline *pipeline_merge;
	vulkan::RenderPass *render_pass_merge;

	bool start_frame_merge();
	void end_frame_merge();

	vulkan::FrameBuffer *_cfb;
	vulkan::FrameBuffer *current_frame_buffer() override;
};


#endif /* SRC_RENDERER_GBUFFERRENDERER_H_ */
