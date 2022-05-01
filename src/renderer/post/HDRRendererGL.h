/*
 * HDRRendererGL.h
 *
 *  Created on: 23 Nov 2021
 *      Author: michi
 */

#pragma once

#include "PostProcessor.h"
#ifdef USING_OPENGL

class vec2;

class HDRRendererGL : public PostProcessorStage {
public:
	HDRRendererGL(Renderer *parent);
	virtual ~HDRRendererGL();

	void prepare() override;
	void draw() override;

	void process_blur(FrameBuffer *source, FrameBuffer *target, float threshold, const vec2 &axis);
	void process(const Array<Texture*> &source, FrameBuffer *target, Shader *shader);
	void render_out(FrameBuffer *source, Texture *bloom);

	shared<FrameBuffer> fb_main;
	shared<FrameBuffer> fb_main_ms;
	shared<FrameBuffer> fb_small1;
	shared<FrameBuffer> fb_small2;

	FrameBuffer *frame_buffer() const override { return fb_main.get(); };
	DepthBuffer *depth_buffer() const override { return _depth_buffer; };
	bool forwarding_into_window() const override { return false; };

	DepthBuffer *_depth_buffer = nullptr;
	shared<Shader> shader_blur;
	shared<Shader> shader_out;
	shared<Shader> shader_resolve_multisample;

	VertexBuffer *vb_2d;

	int ch_post_blur = -1, ch_out = -1;
};

#endif
