/*
 * TextureRenderer.h
 *
 *  Created on: 05.01.2020
 *      Author: michi
 */

#ifndef SRC_RENDERER_TEXTURERENDERER_H_
#define SRC_RENDERER_TEXTURERENDERER_H_

#include "Renderer.h"

class TextureRenderer : public Renderer {
public:
	TextureRenderer(vulkan::Texture *tex);
	~TextureRenderer() override;

	vulkan::Texture *tex;

	vulkan::DepthBuffer *depth_buffer;
	vulkan::FrameBuffer *frame_buffer;

	bool start_frame() override;
	void end_frame() override;

	vulkan::FrameBuffer *current_frame_buffer() override;
};



#endif /* SRC_RENDERER_TEXTURERENDERER_H_ */
