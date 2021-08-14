/*
 * RenderPathGLForward.h
 *
 *  Created on: Jun 2, 2021
 *      Author: michi
 */

#pragma once

#include "RenderPathGL.h"

class Camera;
class PerformanceMonitor;

class RenderPathGLForward : public RenderPathGL {
public:

	RenderPathGLForward(GLFWwindow* win, int w, int h);
	void draw() override;

	void render_into_texture(nix::FrameBuffer *fb, Camera *cam, const rect &target_area) override;
	void draw_world(bool allow_material);
	void render_shadow_map(nix::FrameBuffer *sfb, float scale);
};

