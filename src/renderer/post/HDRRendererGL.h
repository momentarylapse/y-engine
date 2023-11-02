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
class Camera;

class HDRRendererGL : public PostProcessorStage {
public:
	HDRRendererGL(Renderer *parent, Camera *cam);
	virtual ~HDRRendererGL();

	void prepare(const RenderParams& params) override;
	void draw(const RenderParams& params) override;

	void process_blur(FrameBuffer *source, FrameBuffer *target, float r, float threshold, const vec2 &axis);
	void process(const Array<Texture*> &source, FrameBuffer *target, Shader *shader);
	void render_out(FrameBuffer *source, Texture *bloom, const RenderParams& params);

	Camera *cam;

	shared<FrameBuffer> fb_main;
	shared<FrameBuffer> fb_main_ms;

	static const int MAX_BLOOM_LEVELS = 4;

	struct BloomLevel {
		shared<FrameBuffer> fb_temp;
		shared<FrameBuffer> fb_out;
	} bloom_levels[MAX_BLOOM_LEVELS];

	FrameBuffer *frame_buffer() const override { return fb_main.get(); };
	DepthBuffer *depth_buffer() const override { return _depth_buffer; };

	DepthBuffer *_depth_buffer = nullptr;
	shared<Shader> shader_blur;
	shared<Shader> shader_out;
	shared<Shader> shader_resolve_multisample;

	owned<VertexBuffer> vb_2d;

	int ch_post_blur = -1, ch_out = -1;
};

#endif
