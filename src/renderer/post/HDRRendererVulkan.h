/*
 * HDRRendererVulkan.h
 *
 *  Created on: 23 Nov 2021
 *      Author: michi
 */


#pragma once

#include "PostProcessor.h"
#ifdef USING_VULKAN

class HDRRendererVulkan : public PostProcessorStage {
public:
	HDRRendererVulkan(Renderer *parent);
	virtual ~HDRRendererVulkan();

	void prepare() override;
	void draw() override;

	void process_blur(CommandBuffer *cb, FrameBuffer *source, FrameBuffer *target, float threshold, int axis);

	struct RenderIntoData {
		RenderIntoData() {}
		RenderIntoData(Renderer *r);
		void render_into(Renderer *r);

		shared<FrameBuffer> fb_main;
		DepthBuffer *_depth_buffer = nullptr;

		//shared<RenderPass> render_pass;
		RenderPass *_render_pass = nullptr;
	} into;


	struct RenderOutData {
		RenderOutData(){}
		RenderOutData(Shader *s, Renderer *r, const Array<Texture*> &tex);
		void render_out(CommandBuffer *cb, const Array<float> &data);
		shared<Shader> shader_out;
		Pipeline* pipeline_out;
		DescriptorSet *dset_out;
		VertexBuffer *vb_2d;
	} out;


	FrameBuffer *fb_main;
	shared<FrameBuffer> fb_small1;
	shared<FrameBuffer> fb_small2;

	FrameBuffer *frame_buffer() const override { return into.fb_main.get(); };
	DepthBuffer *depth_buffer() const override { return into._depth_buffer; };
	bool forwarding_into_window() const override { return false; };

	shared<Shader> shader_blur;

	VertexBuffer *vb_2d;

	int ch_post_blur = -1, ch_out = -1;
};

#endif
