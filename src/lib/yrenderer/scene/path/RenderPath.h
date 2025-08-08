/*
 * RenderPath.h
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
class CubeMapRenderer;
class CubeMapSource;



class RenderPath : public yrenderer::Renderer {
public:
	RenderPath(yrenderer::Context* ctx, const string &name, yrenderer::SceneView& scene_view);
	~RenderPath() override;

	yrenderer::CameraParams view;
	bool wireframe = false;
	float ambient_occlusion_radius = 0;

	yrenderer::SceneView& scene_view;

	owned<yrenderer::ShadowRenderer> shadow_renderer;
	owned<yrenderer::CubeMapRenderer> cube_map_renderer;

	virtual void add_background_emitter(shared<yrenderer::MeshEmitter> emitter) {}
	virtual void add_opaque_emitter(shared<yrenderer::MeshEmitter> emitter) {}
	virtual void add_transparent_emitter(shared<yrenderer::MeshEmitter> emitter) {}


	void create_shadow_renderer(int resolution);
	void create_cube_renderer();

	void set_lights(const Array<yrenderer::Light*>& lights);

	void reset();

	void render_into_cubemap(const yrenderer::RenderParams& params, yrenderer::CubeMapSource& source);
};

}
