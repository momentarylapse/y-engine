/*
 * RenderPathGLForward.h
 *
 *  Created on: Jun 2, 2021
 *      Author: michi
 */

#pragma once

#include "RenderPathGL.h"


class RenderPathGLForward : public RenderPathGL {
public:

	RenderPathGLForward(GLFWwindow* win, int w, int h, PerformanceMonitor *pm);
	void draw() override;

	void render_into_texture(nix::FrameBuffer *fb, Camera *cam) override;
	void draw_skyboxes(Camera *cam);
	void draw_terrains(bool allow_material);
	void draw_objects(bool allow_material);
	void draw_world(bool allow_material);
	void draw_particles();
	void prepare_lights();
	void render_shadow_map(nix::FrameBuffer *sfb, float scale);
};

