/*
 * WorldRendererGLForward.h
 *
 *  Created on: Jun 2, 2021
 *      Author: michi
 */

#pragma once

#include "WorldRenderer.h"

namespace yrenderer {
	class SceneRenderer;
}
class Profiler;

class WorldRendererForward : public WorldRenderer {
public:
	explicit WorldRendererForward(yrenderer::Context* ctx, yrenderer::SceneView& scene_view, int shadow_resolution);

	void add_background_emitter(shared<yrenderer::MeshEmitter> emitter) override;
	void add_opaque_emitter(shared<yrenderer::MeshEmitter> emitter) override;
	void add_transparent_emitter(shared<yrenderer::MeshEmitter> emitter) override;

	void prepare(const yrenderer::RenderParams& params) override;
	void draw(const yrenderer::RenderParams& params) override;

	yrenderer::SceneRenderer* scene_renderer;
};
