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

	nix::set_viewport(params.area);
	draw(RenderParams::into_texture(frame_buffer.get(), params.desired_aspect_ratio));
}
#endif
