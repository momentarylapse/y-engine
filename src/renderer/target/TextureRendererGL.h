/*
 * TextureRendererGL.h
 *
 *  Created on: Nov 10, 2023
 *      Author: michi
 */

#pragma once


#include "../Renderer.h"
#ifdef USING_OPENGL
#include <lib/base/optional.h>
#include <lib/image/color.h>

class TextureRenderer : public RenderTask {
public:
	explicit TextureRenderer(const string& name, const shared_array<Texture>& textures, const Array<string>& options = {});

	// TODO move to explicit/dependency graph
	void prepare(const RenderParams& params) override;

	void render(const RenderParams& params) override;

	void set_area(const rect& area);
	bool override_area = false;
	rect user_area;

	shared<FrameBuffer> frame_buffer;
	bool clear_z = true;
	base::optional<color> clear_color;
};

#endif
