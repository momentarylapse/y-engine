/*
 * ShadowRendererGL.h
 *
 *  Created on: Dec 11, 2022
 *      Author: michi
 */

#pragma once

#include "../../Renderer.h"
#include "../../../graphics-fwd.h"
#ifdef USING_OPENGL
#include "../../../lib/math/mat4.h"

class Camera;
class PerformanceMonitor;

class ShadowRendererGL : public Renderer {
public:
	ShadowRendererGL(Renderer *parent);

	shared<FrameBuffer> fb[2];

    void render_shadow_map(FrameBuffer *sfb, float scale);

	void prepare() override;
	void draw() override {}

    void render(const mat4 &m);

    mat4 shadow_proj;
};

#endif
