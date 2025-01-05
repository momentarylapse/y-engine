/*
 * WorldRendererDeferred.h
 *
 *  Created on: 07.08.2020
 *      Author: michi
 */

#pragma once

#include "WorldRenderer.h"
#include "geometry/RenderViewData.h"

class ThroughShaderRenderer;

class WorldRendererDeferred : public WorldRenderer {
public:

	shared_array<Texture> gbuffer_textures;
	shared<FrameBuffer> gbuffer;
	UniformBuffer *ssao_sample_buffer;
	int ch_gbuf_out = -1;
	int ch_trans = -1;

	owned<GeometryRenderer> geo_renderer_trans;
	ThroughShaderRenderer* out_renderer;

	WorldRendererDeferred(Camera *cam, SceneView& scene_view, int width, int height);
	void prepare(const RenderParams& params) override;
	void draw(const RenderParams& params) override;

	//void render_into_texture(Camera *cam, RenderViewData &rvd, const RenderParams& params) override;
	void render_into_gbuffer(FrameBuffer *fb, const RenderParams& params);
	void draw_background(const RenderParams& params);


	void render_out_from_gbuffer(FrameBuffer *source, const RenderParams& params);

#ifdef USING_VULKAN
	owned<RenderPass> render_pass;
#endif
};
