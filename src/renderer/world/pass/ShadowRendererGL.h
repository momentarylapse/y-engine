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
struct SceneView;

class ShadowRenderer : public RenderTask {
public:
	ShadowRenderer();

	shared<FrameBuffer> fb[2];

    void render_shadow_map(FrameBuffer *sfb, float scale, RenderViewData& rvd);

	void prepare(const RenderParams& params) override {};
	void draw(const RenderParams& params) override {}

	void set_scene(SceneView &parent_scene_view);
    void render(const RenderParams& params) override;

    owned<Material> material;
	mat4 proj;
	SceneView scene_view;
	RenderViewData rvd[2];

    owned<GeometryRendererGL> geo_renderer;
};

#endif
