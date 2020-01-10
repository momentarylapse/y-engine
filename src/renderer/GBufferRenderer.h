/*
 * GBufferRenderer.h
 *
 *  Created on: 05.01.2020
 *      Author: michi
 */

#ifndef SRC_RENDERER_GBUFFERRENDERER_H_
#define SRC_RENDERER_GBUFFERRENDERER_H_


#include "Renderer.h"


// for creating g-buffers
class GBufferRenderer : public Renderer {
public:
	GBufferRenderer(int w, int h);
	~GBufferRenderer() override;

	void _create(int w, int h);
	void _destroy();
	void resize(int w, int h);

	vulkan::Texture *tex_emission;
	vulkan::Texture *tex_color;
	vulkan::Texture *tex_pos;
	vulkan::Texture *tex_normal;

	vulkan::DepthBuffer *depth_buffer;
	vulkan::FrameBuffer *g_buffer;

	vulkan::Shader *shader_into_gbuf;
	vulkan::Pipeline *pipeline_into_gbuf;

	vulkan::RenderPass *_default_render_pass;

	bool start_frame() override;
	void end_frame() override;
	vulkan::RenderPass *default_render_pass() override { return _default_render_pass; }
	vulkan::FrameBuffer *current_frame_buffer() override;
};


#endif /* SRC_RENDERER_GBUFFERRENDERER_H_ */
