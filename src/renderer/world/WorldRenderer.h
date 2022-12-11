/*
 * WorldRenderer.h
 *
 *  Created on: Jan 19, 2020
 *      Author: michi
 */

#pragma once

#include "../Renderer.h"
#include "../../graphics-fwd.h"
#include "../../lib/math/mat4.h"
#include "../../lib/math/vec3.h"
#include "../../lib/image/color.h"
#include "../../lib/base/callable.h"
#include "../../lib/base/pointer.h"

class ShadowMapRenderer;
class PerformanceMonitor;
class World;
class Camera;
class mat4;
class vec3;
class quaternion;
class Material;
class UBOLight;

mat4 mtr(const vec3 &t, const quaternion &a);



struct UBOMatrices {
	alignas(16) mat4 model;
	alignas(16) mat4 view;
	alignas(16) mat4 proj;
};

struct UBOFog {
	alignas(16) color col;
	alignas(16) float distance;
};

struct VertexFx {
	vec3 pos;
	color col;
	float u, v;
};


enum class RenderPathType {
	NONE,
	FORWARD,
	DEFERRED
};

struct RenderInjector {
	using Callback = Callable<void()>;
	const Callback *func;
	bool transparent;
	bool shadow;
};

class WorldRenderer : public Renderer {
public:
	WorldRenderer(const string &name, Renderer *parent);
	virtual ~WorldRenderer();

	color background() const override;

	int ch_post = -1, ch_post_focus = -1;
	int ch_pre = -1, ch_bg = -1, ch_fx = -1, ch_world = -1, ch_prepare_lights = -1;

	RenderPathType type = RenderPathType::NONE;

	shared<FrameBuffer> fb_shadow1;
	shared<FrameBuffer> fb_shadow2;
	Material *material_shadow = nullptr;

	shared<Shader> shader_fx;

	Array<UBOLight> lights;

	shared<DepthBuffer> depth_cube;
	shared<FrameBuffer> fb_cube;
	shared<CubeMap> cube_map;

	//Camera *shadow_cam;
	mat4 shadow_proj;
	int shadow_index;

	float shadow_box_size;
	int shadow_resolution;

	bool wireframe = false;


	bool using_view_space = false;


	Array<RenderInjector> fx_injectors;
	void add_fx_injector(const RenderInjector::Callback *f, bool transparent);

	void reset();
};


