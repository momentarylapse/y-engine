/*
 * PostProcessorGL.h
 *
 *  Created on: Dec 7, 2021
 *      Author: michi
 */


#pragma once

#include "../Renderer.h"
#ifdef USING_OPENGL
#include "../../graphics-fwd.h"
#include "../../lib/base/callable.h"

class vec2;
class Any;

struct PostProcessorStage : public Renderer {
	using Callback = Callable<FrameBuffer*(FrameBuffer*)>;
	const Callback *func;
	int channel;
};

class PostProcessorGL : public Renderer {
public:
	PostProcessorGL(Renderer *parent);
	virtual ~PostProcessorGL();

	void prepare() override;
	void draw() override;

	Array<PostProcessorStage*> stages;
	void add_stage(const PostProcessorStage::Callback *f);
	void reset();

	void process(const Array<Texture*> &source, FrameBuffer *target, Shader *shader, const Any &data);
	FrameBuffer* do_post_processing(FrameBuffer *source);

	shared<Shader> shader_depth;
	shared<Shader> shader_resolve_multisample;
	void process_blur(FrameBuffer *source, FrameBuffer *target, float threshold, const vec2 &axis);
	void process_depth(FrameBuffer *source, FrameBuffer *target, const vec2 &axis);
	//void process(const Array<Texture*> &source, FrameBuffer *target, Shader *shader);
	//FrameBuffer* do_post_processing(FrameBuffer *source);
	FrameBuffer* resolve_multisampling(FrameBuffer *source);

	shared<FrameBuffer> fb1;
	shared<FrameBuffer> fb2;
	FrameBuffer *next_fb(FrameBuffer *cur);

	FrameBuffer *frame_buffer() const override;
	DepthBuffer *depth_buffer() const override;
	bool forwarding_into_window() const override;

	DepthBuffer *_depth_buffer = nullptr;
	shared<Shader> shader_blur;
	shared<Shader> shader_out;

	VertexBuffer *vb_2d;

	int ch_post_blur = -1, ch_out = -1;
};

#endif
