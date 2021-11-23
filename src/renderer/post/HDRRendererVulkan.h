/*
 * HDRRendererVulkan.h
 *
 *  Created on: 23 Nov 2021
 *      Author: michi
 */


#pragma once

#include "../Renderer.h"
#ifdef USING_VULKAN

class HDRRendererVulkan : public Renderer {
public:
	HDRRendererVulkan(Renderer *parent);
	virtual ~HDRRendererVulkan();

	void prepare() override;
	void draw() override;


	void process_blur(CommandBuffer *cb, FrameBuffer *source, FrameBuffer *target, float threshold, int axis);
	void render_out(CommandBuffer *cb, FrameBuffer *source, Texture *bloom);

	//shared<RenderPass> render_pass;
	RenderPass *render_pass = nullptr;


	shared<FrameBuffer> fb_main;
	DepthBuffer *depth_buffer = nullptr;
	shared<FrameBuffer> fb_small1;
	shared<FrameBuffer> fb_small2;

	FrameBuffer *current_frame_buffer() const { return fb_main.get(); };
	DepthBuffer *current_depth_buffer() const { return depth_buffer; };

	shared<Shader> shader_blur;
	shared<Shader> shader_out;
	Pipeline* pipeline_out;
	DescriptorSet *dset_out;

	VertexBuffer *vb_2d;

	int ch_post_blur = -1, ch_out = -1;
};

#endif
