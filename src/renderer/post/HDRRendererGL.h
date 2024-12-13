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
class ComputeTask;
class TextureRenderer;
class ThroughShaderRenderer;
class MultisampleResolver;

class HDRRendererGL : public PostProcessorStage {
public:
	HDRRendererGL(Camera *cam, const shared<Texture>& tex, const shared<DepthBuffer>& depth_buffer);
	~HDRRendererGL() override;

	void prepare(const RenderParams& params) override;
	void draw(const RenderParams& params) override;

	Camera *cam;

	owned<MultisampleResolver> ms_resolver;

	owned<TextureRenderer> texture_renderer;
	shared<FrameBuffer> fb_main;
	shared<Texture> tex_main;

	owned<ThroughShaderRenderer> out_renderer;

	static const int MAX_BLOOM_LEVELS = 4;

	struct BloomLevel {
		shared<Texture> tex_temp;
		shared<Texture> tex_out;
		owned<TextureRenderer> renderer[2];
		owned<ThroughShaderRenderer> tsr[2];
	} bloom_levels[MAX_BLOOM_LEVELS];

	shared<DepthBuffer> _depth_buffer;
	shared<Shader> shader_blur;
	shared<Shader> shader_out;

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
