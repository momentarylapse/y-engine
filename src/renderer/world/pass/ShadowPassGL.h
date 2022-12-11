/*
 * ShadowPassGL.h
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

class ShadowPassGL : public Renderer {
public:
	ShadowPassGL(Renderer *parent);

    void set(const mat4 &shadow_proj, float scale);

	void prepare() override;
	void draw() override;

    float scale;
    mat4 shadow_proj;
};

#endif
