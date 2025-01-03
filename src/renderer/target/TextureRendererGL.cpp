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

TextureRenderer::TextureRenderer(const shared_array<Texture>& textures, const Array<string>& options) : RenderTask("tex") {
	frame_buffer = new FrameBuffer(textures);
}

void TextureRenderer::render(const RenderParams& params) {
	nix::bind_frame_buffer(frame_buffer.get());

	auto area = frame_buffer->area();
	if (use_params_area)
		area = params.area;

	nix::set_viewport(area);
	if (clear_z)
		nix::clear_z();
	draw(RenderParams::into_texture(frame_buffer.get(), params.desired_aspect_ratio).with_area(area));
}

void TextureRenderer::prepare(const RenderParams& params) {
	Renderer::prepare(params);

	render(params);
}

#endif
