/*
 * WorldRendererGLDeferred.h
 *
 *  Created on: 07.08.2020
 *      Author: michi
 */

#pragma once

#include "WorldRendererGL.h"
#ifdef USING_OPENGL

class ShadowRendererGL;

class WorldRendererGLDeferred : public WorldRendererGL {
public:

	shared<FrameBuffer> gbuffer;
	shared<Shader> shader_gbuffer_out;
	UniformBuffer *ssao_sample_buffer;
	int ch_gbuf_out = -1;
	int ch_trans = -1;

	WorldRendererGLDeferred(Renderer *parent);
	void prepare() override;
	void draw() override;

	bool forwarding_into_window() const override { return false; };

	//void render_into_texture(FrameBuffer *fb, Camera *cam) override;
	void render_into_gbuffer(FrameBuffer *fb, Camera *cam);
	void draw_background(FrameBuffer *fb, Camera *cam);
	void draw_world(bool allow_material);


	void render_out_from_gbuffer(FrameBuffer *source);

	ShadowRendererGL *shadow_renderer = nullptr;
};

#endif
