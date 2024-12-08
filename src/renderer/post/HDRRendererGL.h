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

class HDRRendererGL : public PostProcessorStage {
public:
	HDRRendererGL(Camera *cam, int width, int height);
	~HDRRendererGL() override;

	void prepare(const RenderParams& params) override;
	void draw(const RenderParams& params) override;

	void process_blur(FrameBuffer *source, FrameBuffer *target, float r, float threshold, const vec2 &axis);
	void process(const Array<Texture*> &source, FrameBuffer *target, Shader *shader);

	Camera *cam;

	owned<TextureRenderer> texture_renderer;
	shared<FrameBuffer> fb_main;
	shared<FrameBuffer> fb_main_ms;

	owned<ThroughShaderRenderer> out_renderer;
	/*owned<TextureRenderer> resolve_ms_renderer;
	owned<ThroughShaderRenderer> resolve_ms_renderer;*/

	static const int MAX_BLOOM_LEVELS = 4;

	struct BloomLevel {
		shared<Texture> tex_temp;
		shared<Texture> tex_out;
		owned<TextureRenderer> renderer[2];
		owned<ThroughShaderRenderer> tsr[2];
	} bloom_levels[MAX_BLOOM_LEVELS];

	DepthBuffer *_depth_buffer = nullptr;
	shared<Shader> shader_blur;
	shared<Shader> shader_out;
	shared<Shader> shader_resolve_multisample;

	owned<VertexBuffer> vb_2d;

	int ch_post_blur = -1, ch_out = -1;

	struct LightMeter {
		void init(ResourceManager* resource_manager, FrameBuffer* frame_buffer, int channel);
		ComputeTask* compute;
		UniformBuffer* params;
		ShaderStorageBuffer* buf;
		Array<int> histogram;
		float brightness;
		int ch_post_brightness = -1;
		void measure(FrameBuffer* frame_buffer);
		void adjust_camera(Camera* cam);
	} light_meter;
};

#endif
