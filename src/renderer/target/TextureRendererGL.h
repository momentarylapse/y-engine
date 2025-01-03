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
	explicit TextureRenderer(const shared_array<Texture>& textures, const Array<string>& options = {});

	// TODO move to explicit/dependency graph
	void prepare(const RenderParams& params) override;

	void render(const RenderParams& params) override;

	shared<FrameBuffer> frame_buffer;
	bool use_params_area = true;
	bool clear_z = true;
};

#endif
