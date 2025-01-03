/*
 * WorldRendererGLForward.h
 *
 *  Created on: Jun 2, 2021
 *      Author: michi
 */

#pragma once

#include "WorldRendererGL.h"
#ifdef USING_OPENGL
#include "geometry/RenderViewData.h"

class Camera;
class PerformanceMonitor;

class WorldRendererGLForward : public WorldRendererGL {
public:
	WorldRendererGLForward(Camera *cam, SceneView& scene_view, RenderViewData& main_rvd);

	void prepare(const RenderParams& params) override;
	void draw(const RenderParams& params) override;

	void draw_with(const RenderParams& params, RenderViewData& rvd);

	RenderViewData& main_rvd;
};

#endif
