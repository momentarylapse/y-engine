/*
 * RenderPathGLForward.h
 *
 *  Created on: Jun 2, 2021
 *      Author: michi
 */

#pragma once

#include "RenderPathGL.h"
#ifdef USING_OPENGL

class Camera;
class PerformanceMonitor;

class RenderPathGLForward : public RenderPathGL {
public:

	RenderPathGLForward(RendererGL *renderer);
	void draw() override;

	void render_into_texture(FrameBuffer *fb, Camera *cam, const rect &target_area) override;
	void draw_world(bool allow_material);
	void render_shadow_map(FrameBuffer *sfb, float scale);
};

#endif
