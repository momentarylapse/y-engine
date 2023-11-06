/*
 * HDRRendererVulkan.h
 *
 *  Created on: 23 Nov 2021
 *      Author: michi
 */


#pragma once

#include "PostProcessor.h"
#ifdef USING_VULKAN

class Camera;

class HDRRendererVulkan : public PostProcessorStage {
public:
	HDRRendererVulkan(Renderer *parent, Camera* cam, int width, int height);
	virtual ~HDRRendererVulkan();

	void prepare(const RenderParams& params) override;
	void draw(const RenderParams& params) override;

	void process_blur(CommandBuffer *cb, FrameBuffer *source, FrameBuffer *target, float threshold, int axis);

	struct RenderIntoData {
		RenderIntoData() {}
		RenderIntoData(int width, int height);
		void render_into(Renderer *r, const RenderParams& params);

		shared<FrameBuffer> fb_main;
		DepthBuffer *_depth_buffer = nullptr;

		//shared<RenderPass> render_pass;
		RenderPass *_render_pass = nullptr;
	} into;


	struct RenderOutData {
		RenderOutData(){}
		RenderOutData(Shader *s, RenderPass *render_pass, const Array<Texture*> &tex);
		void render_out(CommandBuffer *cb, const Array<float> &data, float exposure, const RenderParams& params);
		shared<Shader> shader_out;
		GraphicsPipeline* pipeline_out;
		DescriptorSet *dset_out;
		VertexBuffer *vb_2d;
	} out;

	Camera *cam;


	FrameBuffer *fb_main;

	static const int MAX_BLOOM_LEVELS = 4;

	struct BloomLevel {
		shared<FrameBuffer> fb_temp;
		shared<FrameBuffer> fb_out;
	} bloom_levels[MAX_BLOOM_LEVELS];

	shared<Shader> shader_blur;
	shared<Shader> shader_out;

	owned<VertexBuffer> vb_2d;

	int ch_post_blur = -1, ch_out = -1;

	RenderPass *get_render_pass() override {
		return into._render_pass;
	}
};

#endif
