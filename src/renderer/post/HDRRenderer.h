/*
 * HDRRenderer.h
 *
 *  Created on: 23 Nov 2021
 *      Author: michi
 */

#pragma once

#include "../Renderer.h"

class vec2;
class Camera;
class ComputeTask;
class TextureRenderer;
class ThroughShaderRenderer;
class MultisampleResolver;
class LightMeter;

class HDRRenderer : public Renderer {
public:
	HDRRenderer(Camera *cam, const shared<Texture>& tex, const shared<DepthBuffer>& depth_buffer);
	~HDRRenderer() override;

	void prepare(const RenderParams& params) override;
	void draw(const RenderParams& params) override;

	Camera *cam;

	owned<MultisampleResolver> ms_resolver;

	owned<TextureRenderer> texture_renderer;
	shared<FrameBuffer> fb_main;
	shared<Texture> tex_main;

	owned<ThroughShaderRenderer> out_renderer;

	static constexpr int MAX_BLOOM_LEVELS = 4;

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




#ifdef USING_VULKAN

	void process_blur(CommandBuffer *cb, FrameBuffer *source, FrameBuffer *target, float threshold, int axis);

	owned<VertexBuffer> vb_2d;
	rect vb_2d_current_source = rect::EMPTY;

#endif



	LightMeter* light_meter;
};

