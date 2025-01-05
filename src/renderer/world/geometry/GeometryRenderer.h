/*
 * GeometryRenderer.h
 *
 *  Created on: Dec 15, 2022
 *      Author: michi
 */

#pragma once

#include "../../Renderer.h"
#include "RenderViewData.h"
#include "../../../graphics-fwd.h"
#include <lib/math/vec3.h>
#include <lib/image/color.h>
#include <lib/math/mat4.h>
#include "../../../world/Light.h"
#include "../../../world/Material.h"

class Camera;
class PerformanceMonitor;
class Material;
class UBOLight;
struct SceneView;
class RenderViewData;

enum class RenderPathType;

mat4 mtr(const vec3 &t, const quaternion &a);

static constexpr int MAX_LIGHTS = 1024;


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

class GeometryRendererCommon : public Renderer {
public:
	enum class Flags {
		ALLOW_OPAQUE = 1,
		ALLOW_TRANSPARENT = 2,
		ALLOW_SKYBOXES = 4,
		ALLOW_CLEAR_COLOR = 8,
		SHADOW_PASS = 1024,
	} flags;

	GeometryRendererCommon(RenderPathType type, SceneView &scene_view);

	void set(Flags flags);
	bool is_shadow_pass() const;

	RenderViewData cur_rvd;

	int ch_pre, ch_bg, ch_fx, ch_terrains, ch_models, ch_user, ch_prepare_lights;

	static constexpr bool using_view_space = true;
	RenderPathType type;

	Material *material_shadow = nullptr; // ref to ShadowRenderer

	SceneView &scene_view;

	shared<Shader> shader_fx;
	shared<Shader> shader_fx_points;
	owned<VertexBuffer> vb_fx;
	owned<VertexBuffer> vb_fx_points;

	base::map<Material*, ShaderCache> multi_pass_shader_cache[4];
	// material as id!
	Shader* get_shader(Material* material, int pass_no, const string& vertex_shader_module, const string& geometry_shader_module);
	Material fx_material;

	owned_array<VertexBuffer> fx_vertex_buffers;
};

inline GeometryRendererCommon::Flags operator|(GeometryRendererCommon::Flags a, GeometryRendererCommon::Flags b) {
	return (GeometryRendererCommon::Flags)((int)a | (int)b);
}

inline GeometryRendererCommon::Flags operator&(GeometryRendererCommon::Flags a, GeometryRendererCommon::Flags b) {
	return (GeometryRendererCommon::Flags)((int)a & (int)b);
}
