/*
 * WorldRenderer.h
 *
 *  Created on: Jan 19, 2020
 *      Author: michi
 */

#pragma once

#include <lib/yrenderer/Renderer.h>
#include <lib/yrenderer/scene/SceneView.h>
#include <lib/base/pointer.h>

namespace yrenderer {
	class ShadowMapRenderer;
	class MeshEmitter;
	class ShadowRenderer;
}



class WorldRenderer : public yrenderer::Renderer {
public:
	WorldRenderer(yrenderer::Context* ctx, const string &name, yrenderer::SceneView& scene_view);
	~WorldRenderer() override;

	yrenderer::CameraParams view;
	bool wireframe = false;

	yrenderer::SceneView& scene_view;

	owned<yrenderer::ShadowRenderer> shadow_renderer;

	virtual void add_background_emitter(shared<yrenderer::MeshEmitter> emitter) {}
	virtual void add_opaque_emitter(shared<yrenderer::MeshEmitter> emitter) {}
	virtual void add_transparent_emitter(shared<yrenderer::MeshEmitter> emitter) {}


	void create_shadow_renderer(int resolution);

	void reset();
};


