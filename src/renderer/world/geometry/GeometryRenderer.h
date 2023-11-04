/*
 * GeometryRenderer.h
 *
 *  Created on: Dec 15, 2022
 *      Author: michi
 */

#pragma once

#include "../../Renderer.h"
#include "../../../graphics-fwd.h"
#include "../../../lib/math/vec3.h"
#include "../../../lib/image/color.h"
#include "../../../lib/math/mat4.h"
#include "../../../world/Light.h"
#include "../../../world/Material.h"

class Camera;
class PerformanceMonitor;
class Material;
class UBOLight;
struct SceneView;

enum class RenderPathType;

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

struct VertexPoint {
	vec3 pos;
	float radius;
	color col;
};

class GeometryRenderer : public Renderer {
public:
	enum class Flags {
		ALLOW_OPAQUE = 1,
		ALLOW_TRANSPARENT = 2,
		SHADOW_PASS = 4,
	} flags;

	GeometryRenderer(RenderPathType type, SceneView &scene_view, Renderer *parent);

	bool is_shadow_pass() const;

	int ch_pre = -1, ch_bg = -1, ch_fx = -1, ch_world = -1, ch_prepare_lights = -1;

	static bool using_view_space;
	RenderPathType type;

	owned<Material> material_shadow;

	SceneView &scene_view;

	shared<Shader> shader_fx;
	shared<Shader> shader_fx_points;
	owned<VertexBuffer> vb_fx;
	owned<VertexBuffer> vb_fx_points;
};

inline GeometryRenderer::Flags operator|(GeometryRenderer::Flags a, GeometryRenderer::Flags b) {
	return (GeometryRenderer::Flags)((int)a | (int)b);
}

inline GeometryRenderer::Flags operator&(GeometryRenderer::Flags a, GeometryRenderer::Flags b) {
	return (GeometryRenderer::Flags)((int)a & (int)b);
}
