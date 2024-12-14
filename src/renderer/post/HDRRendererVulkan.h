/*
 * HDRRendererVulkan.h
 *
 *  Created on: 23 Nov 2021
 *      Author: michi
 */


#pragma once

#include "PostProcessor.h"
#ifdef USING_VULKAN
#include <lib/math/rect.h>

class Camera;
class ComputeTask;
class TextureRenderer;
class ThroughShaderRenderer;
class MultisampleResolver;

class HDRRendererVulkan : public PostProcessorStage {
public:
	HDRRendererVulkan(Camera* cam, const shared<Texture>& tex, const shared<DepthBuffer>& depth_buffer);
	~HDRRendererVulkan() override;

	void prepare(const RenderParams& params) override;
	void draw(const RenderParams& params) override;

	void process_blur(CommandBuffer *cb, FrameBuffer *source, FrameBuffer *target, float threshold, int axis);


	struct RenderOutData {
		RenderOutData() = default;
		RenderOutData(Shader *s, const Array<Texture*> &tex);
		void render_out(CommandBuffer *cb, const Array<float> &data, float exposure, const RenderParams& params);
		shared<Shader> shader_out;
		GraphicsPipeline* pipeline_out = nullptr;
		DescriptorSet *dset_out;
		VertexBuffer *vb_2d;
		rect vb_2d_current_source = rect::EMPTY;
	} out;

	Camera *cam;
	TextureRenderer* texture_renderer;
	MultisampleResolver* ms_resolver;


	FrameBuffer *fb_main;
	shared<Texture> tex_main;
	shared<DepthBuffer> _depth_buffer;

	static const int MAX_BLOOM_LEVELS = 4;

	struct BloomLevel {
		shared<FrameBuffer> fb_temp;
		shared<FrameBuffer> fb_out;

		shared<Texture> tex_temp;
		shared<Texture> tex_out;
		owned<TextureRenderer> renderer[2];
		owned<ThroughShaderRenderer> tsr[2];
	} bloom_levels[MAX_BLOOM_LEVELS];

	shared<Shader> shader_blur;
	shared<Shader> shader_out;

	owned<VertexBuffer> vb_2d;
	rect vb_2d_current_source = rect::EMPTY;

	int ch_post_blur = -1, ch_out = -1;

	struct LightMeter {
		void init(ResourceManager* resource_manager, Texture* tex, int channel);
		ComputeTask* compute;
		UniformBuffer* params;
		ShaderStorageBuffer* buf;
		Array<int> histogram;
		float brightness;
		int ch_post_brightness = -1;
		void measure(const RenderParams& params, Texture* tex);
		void adjust_camera(Camera* cam);
	} light_meter;
};

#endif
