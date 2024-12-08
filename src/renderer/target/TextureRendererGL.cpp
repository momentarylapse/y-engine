/*
 * TextureRendererGL.cpp
 *
 *  Created on: Nov 23, 2021
 *      Author: michi
 */

#include "TextureRendererGL.h"
#ifdef USING_OPENGL
#include <graphics-impl.h>
//#include "../../helper/PerformanceMonitor.h"

TextureRenderer::TextureRenderer(const shared_array<Texture>& textures) : RenderTask("tex") {
	frame_buffer = new FrameBuffer(textures);
}

void TextureRenderer::render(const RenderParams& params) {
	nix::bind_frame_buffer(frame_buffer.get());

	auto area = frame_buffer->area();
	if (use_params_area)
		area = params.area;

	nix::set_viewport(area);
	draw(RenderParams::into_texture(frame_buffer.get(), params.desired_aspect_ratio).with_area(area));
}

void TextureRenderer::prepare(const RenderParams& params) {
	for (auto c: children)
		c->prepare(params);

	render(params);
}

#endif
