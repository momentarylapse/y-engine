/*
 * WorldRendererGLForward.h
 *
 *  Created on: Jun 2, 2021
 *      Author: michi
 */

#pragma once

#include "WorldRendererGL.h"
#ifdef USING_OPENGL

class Camera;
class PerformanceMonitor;

class WorldRendererGLForward : public WorldRendererGL {
public:
	WorldRendererGLForward(Renderer *parent);

	void prepare() override;
	void draw() override;

	void render_into_texture(FrameBuffer *fb, Camera *cam) override;
	void draw_world(bool allow_material);
	void render_shadow_map(FrameBuffer *sfb, float scale);
};

#endif