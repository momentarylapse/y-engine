/*
 * TextureRenderer.h
 *
 *  Created on: 05.01.2020
 *      Author: michi
 */

#ifndef SRC_RENDERER_TEXTURERENDERER_H_
#define SRC_RENDERER_TEXTURERENDERER_H_

#if HAS_LIB_VULKAN

#include "../Renderer.h"

class TextureRenderer : public RendererVulkan {
public:
	TextureRenderer(vulkan::Texture *tex);
	~TextureRenderer() override;

	vulkan::Texture *tex;

	vulkan::DepthBuffer *depth_buffer;
	vulkan::FrameBuffer *frame_buffer;
	vulkan::RenderPass *_default_render_pass;

	bool start_frame() override;
	void end_frame() override;

	vulkan::RenderPass *default_render_pass() override { return _default_render_pass; }
	vulkan::FrameBuffer *current_frame_buffer() override;
};


#endif

#endif /* SRC_RENDERER_TEXTURERENDERER_H_ */
