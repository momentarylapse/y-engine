/*
 * WorldRendererGLDeferred.h
 *
 *  Created on: 07.08.2020
 *      Author: michi
 */

#pragma once

#include "WorldRenderer.h"
#ifdef USING_OPENGL
#include "geometry/RenderViewData.h"

class WorldRendererGLDeferred : public WorldRenderer {
public:

	shared<FrameBuffer> gbuffer;
	shared<Shader> shader_gbuffer_out;
	UniformBuffer *ssao_sample_buffer;
	int ch_gbuf_out = -1;
	int ch_trans = -1;

	RenderViewData main_rvd;

	owned<GeometryRenderer> geo_renderer_trans;

	WorldRendererGLDeferred(Camera *cam, SceneView& scene_view, int width, int height);
	void prepare(const RenderParams& params) override;
	void draw(const RenderParams& params) override;

	//void render_into_texture(Camera *cam, RenderViewData &rvd, const RenderParams& params) override;
	void render_into_gbuffer(FrameBuffer *fb, const RenderParams& params);
	void draw_background(FrameBuffer *fb, const RenderParams& params);


	void render_out_from_gbuffer(FrameBuffer *source, const RenderParams& params);
};

#endif
