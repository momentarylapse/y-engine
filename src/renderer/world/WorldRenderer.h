/*
 * WorldRenderer.h
 *
 *  Created on: Jan 19, 2020
 *      Author: michi
 */

#pragma once

#include <lib/yrenderer/Renderer.h>
#include <lib/ygraphics/graphics-fwd.h>
#include <lib/math/mat4.h>
#include <lib/math/vec3.h>
#include <lib/image/color.h>
#include <lib/base/callable.h>
#include <lib/base/pointer.h>

#include <lib/yrenderer/scene/SceneView.h>

class Profiler;
class World;
class Camera;
struct mat4;
struct vec3;
struct quaternion;
namespace yrenderer {
	class CubeMapSource;
	class GeometryRenderer;
	class ShadowMapRenderer;
	class MeshEmitter;
	class Material;
}



class WorldRenderer : public yrenderer::Renderer {
public:
	WorldRenderer(yrenderer::Context* ctx, const string &name, Camera* cam, yrenderer::SceneView& scene_view);

	Camera* cam;
	bool wireframe = false;

	yrenderer::SceneView& scene_view;

	virtual void add_background_emitter(shared<yrenderer::MeshEmitter> emitter) {}
	virtual void add_opaque_emitter(shared<yrenderer::MeshEmitter> emitter) {}
	virtual void add_transparent_emitter(shared<yrenderer::MeshEmitter> emitter) {}

	void reset();
};


