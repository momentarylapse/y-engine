/*
 * ShadowMapRenderer.h
 *
 *  Created on: 06.01.2020
 *      Author: michi
 */

#ifndef SRC_RENDERER_SHADOWMAPRENDERER_H_
#define SRC_RENDERER_SHADOWMAPRENDERER_H_


#include "Renderer.h"

class ShadowMapRenderer : public Renderer {
public:
	ShadowMapRenderer(const string &shader_filename);
	~ShadowMapRenderer() override;

	vulkan::DepthBuffer *depth_buffer;
	vulkan::FrameBuffer *frame_buffer;

	vulkan::Shader *shader;
	vulkan::Pipeline *pipeline;

	vulkan::RenderPass *_default_render_pass;

	bool start_frame() override;
	void end_frame() override;
	vulkan::RenderPass *default_render_pass() override { return _default_render_pass; }
	vulkan::FrameBuffer *current_frame_buffer() override { return frame_buffer; }

};





#endif /* SRC_RENDERER_SHADOWMAPRENDERER_H_ */
