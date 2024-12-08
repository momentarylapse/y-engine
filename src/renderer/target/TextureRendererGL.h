/*
 * TextureRendererGL.h
 *
 *  Created on: Nov 10, 2023
 *      Author: michi
 */

#pragma once


#include "../Renderer.h"
#ifdef USING_OPENGL

class TextureRenderer : public RenderTask {
public:
	explicit TextureRenderer(const shared_array<Texture>& textures);

	void render(const RenderParams& params) override;

	shared<FrameBuffer> frame_buffer;
};

#endif
