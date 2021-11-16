/*
 * RenderPathGLDeferred.h
 *
 *  Created on: 07.08.2020
 *      Author: michi
 */

#pragma once

#include "RenderPathGL.h"


class RenderPathGLDeferred : public RenderPathGL {
public:

	shared<FrameBuffer> gbuffer;
	shared<Shader> shader_gbuffer_out;
	UniformBuffer *ssao_sample_buffer;
	int ch_gbuf_out = -1;
	int ch_trans = -1;

	RenderPathGLDeferred(GLFWwindow* win, int w, int h);
	void draw() override;

	void render_into_texture(FrameBuffer *fb, Camera *cam, const rect &target_area) override;
	void render_into_gbuffer(FrameBuffer *fb, Camera *cam, const rect &target_area);
	void render_background(FrameBuffer *fb, Camera *cam, const rect &target_area);
	void draw_world(bool allow_material);
	void render_shadow_map(FrameBuffer *sfb, float scale);


	void render_out_from_gbuffer(FrameBuffer *source, FrameBuffer *target);
};


