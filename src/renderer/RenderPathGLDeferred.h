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

	shared<nix::FrameBuffer> gbuffer;
	shared<nix::Shader> shader_gbuffer_out;
	nix::UniformBuffer *ssao_sample_buffer;

	RenderPathGLDeferred(GLFWwindow* win, int w, int h);
	void draw() override;

	void render_into_texture(nix::FrameBuffer *fb, Camera *cam, const rect &target_area) override;
	void render_into_gbuffer(nix::FrameBuffer *fb, Camera *cam, const rect &target_area);
	void render_background(nix::FrameBuffer *fb, Camera *cam, const rect &target_area);
	void draw_world(bool allow_material);
	void render_shadow_map(nix::FrameBuffer *sfb, float scale);


	void render_from_gbuffer(nix::FrameBuffer *source, nix::FrameBuffer *target);
};


