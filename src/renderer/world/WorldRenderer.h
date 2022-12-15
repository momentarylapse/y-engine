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

#include "geometry/GeometryRenderer.h"

class ShadowMapRenderer;
class PerformanceMonitor;
class World;
class Camera;
class mat4;
class vec3;
class quaternion;
class Material;
class UBOLight;


enum class RenderPathType {
	NONE,
	FORWARD,
	DEFERRED
};

class WorldRenderer : public Renderer {
public:
	WorldRenderer(const string &name, Renderer *parent);
	virtual ~WorldRenderer();

	color background() const override;

	int ch_post = -1, ch_post_focus = -1;
	int ch_pre = -1, ch_bg = -1, ch_fx = -1, ch_world = -1, ch_prepare_lights = -1;

	RenderPathType type = RenderPathType::NONE;

	float shadow_box_size;
	int shadow_resolution;

	bool wireframe = false;


	shared<FrameBuffer> fb_shadow1;
	shared<FrameBuffer> fb_shadow2;
	Material *material_shadow = nullptr;
	Camera *cam;

	shared<Shader> shader_fx;

	Array<UBOLight> lights;
	UniformBuffer *ubo_light = nullptr;

	shared<DepthBuffer> depth_cube;
	shared<FrameBuffer> fb_cube;
	shared<CubeMap> cube_map;

	//Camera *shadow_cam;
	mat4 shadow_proj;
	int shadow_index;

	void reset();
};


