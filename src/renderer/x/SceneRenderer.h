//
// Created by Michael Ankele on 2025-05-05.
//

#pragma once

#include "../Renderer.h"
#include "MeshEmitter.h"
#include "../world/geometry/RenderViewData.h"

class MeshEmitter;

class SceneRenderer : public Renderer {
public:
	explicit SceneRenderer(SceneView& scene_view);
	~SceneRenderer() override;

	shared_array<MeshEmitter> emitters;
	void add_emitter(shared<MeshEmitter> emitter);

	void prepare(const RenderParams& params) override;
	void draw(const RenderParams& params) override;

	SceneView& scene_view;
	RenderViewData rvd;

	base::optional<color> background_color;
};


