/*
 * ShadowRendererGL.h
 *
 *  Created on: Dec 11, 2022
 *      Author: michi
 */

#pragma once

#include "../../Renderer.h"
#include "../../../graphics-fwd.h"
#ifdef USING_OPENGL
#include "../geometry/RenderViewData.h"
#include <lib/math/mat4.h>
#include "../geometry/SceneView.h"

class Camera;
class PerformanceMonitor;
class Material;
class GeometryRendererGL;
class TextureRenderer;
struct SceneView;

class ShadowRenderer : public RenderTask {
public:
	ShadowRenderer();

	static constexpr int NUM_CASCADES = 2;



	void prepare(const RenderParams& params) override {};
	void draw(const RenderParams& params) override {}

	void set_scene(SceneView &parent_scene_view);
    void render(const RenderParams& params) override;

    owned<Material> material;
	mat4 proj;
	SceneView scene_view;
	struct Cascade {
		Cascade();
		~Cascade();
		DepthBuffer* depth_buffer = nullptr;
		shared<FrameBuffer> fb;
		RenderViewData rvd;
		owned<TextureRenderer> texture_renderer;
		float scale = 1.0f;
	} cascades[NUM_CASCADES];

    owned<GeometryRendererGL> geo_renderer;

    void render_cascade(Cascade& c);
};

#endif
