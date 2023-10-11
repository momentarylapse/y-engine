/*
 * WorldRendererGLDeferred.h
 *
 *  Created on: 07.08.2020
 *      Author: michi
 */

#pragma once

#include "WorldRendererGL.h"
#ifdef USING_OPENGL

class WorldRendererGLDeferred : public WorldRendererGL {
public:

	shared<FrameBuffer> gbuffer;
	shared<Shader> shader_gbuffer_out;
	UniformBuffer *ssao_sample_buffer;
	int ch_gbuf_out = -1;
	int ch_trans = -1;

	GeometryRendererGL *geo_renderer_trans = nullptr;

	WorldRendererGLDeferred(Renderer *parent, Camera *cam);
	void prepare() override;
	void draw() override;

	bool forwarding_into_window() const override { return false; };

	//void render_into_texture(FrameBuffer *fb, Camera *cam) override;
	void render_into_gbuffer(FrameBuffer *fb);
	void draw_background(FrameBuffer *fb);


	void render_out_from_gbuffer(FrameBuffer *source);
};

#endif
