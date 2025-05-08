/*
 * GeometryRenderer.h
 *
 *  Created on: Dec 15, 2022
 *      Author: michi
 */

#pragma once

#include "../../Renderer.h"
#include "../../scene/RenderViewData.h"
#include "../../../graphics-fwd.h"
#include <lib/math/vec3.h>
#include <lib/image/color.h>
#include <lib/math/mat4.h>
#include "../../../world/Light.h"

class Camera;
class PerformanceMonitor;
class Material;
class UBOLight;
struct SceneView;
class RenderViewData;

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

// generic
class GeometryEmitter : public Renderer {
public:
	GeometryEmitter(RenderPathType type, SceneView &scene_view);

	enum class Flags {
		ALLOW_OPAQUE = 1,
		ALLOW_TRANSPARENT = 2,
		ALLOW_SKYBOXES = 4,
		ALLOW_CLEAR_COLOR = 8,
		SHADOW_PASS = 1024,
	} flags;

	void set(Flags flags);
	bool is_shadow_pass() const;

	RenderViewData cur_rvd;
	RenderPathType type;

	SceneView &scene_view;
	base::optional<vec3> override_view_pos;
	base::optional<quaternion> override_view_ang;
	base::optional<mat4> override_projection;
};

// draw y game scene
// SceneEmitter?
class GeometryRenderer : public GeometryEmitter {
public:
	GeometryRenderer(RenderPathType type, SceneView &scene_view);

	int ch_pre, ch_bg, ch_fx, ch_terrains, ch_models, ch_user, ch_prepare_lights;

	shared<Shader> shader_fx;
	shared<Shader> shader_fx_points;
	owned<VertexBuffer> vb_fx;
	owned<VertexBuffer> vb_fx_points;

	Material fx_material;

	owned_array<VertexBuffer> fx_vertex_buffers;


	void prepare(const RenderParams& params) override;
	void draw(const RenderParams& params) override;


private:
	void clear(const RenderParams& params, RenderViewData &rvd);
	void draw_skyboxes(const RenderParams& params, RenderViewData &rvd);
	void draw_particles(const RenderParams& params, RenderViewData &rvd);
	void draw_terrains(const RenderParams& params, RenderViewData &rvd);
	void draw_objects_opaque(const RenderParams& params, RenderViewData &rvd);
	void draw_objects_transparent(const RenderParams& params, RenderViewData &rvd);
	void draw_objects_instanced(const RenderParams& params, RenderViewData &rvd);
	void draw_user_meshes(const RenderParams& params, RenderViewData &rvd, bool transparent);
	void prepare_instanced_matrices();
	void prepare_lights(Camera *cam, RenderViewData &rvd);

	void draw_opaque(const RenderParams& params, RenderViewData &rvd);
	void draw_transparent(const RenderParams& params, RenderViewData &rvd);
};

inline GeometryEmitter::Flags operator|(GeometryEmitter::Flags a, GeometryEmitter::Flags b) {
	return (GeometryEmitter::Flags)((int)a | (int)b);
}

inline GeometryEmitter::Flags operator&(GeometryEmitter::Flags a, GeometryEmitter::Flags b) {
	return (GeometryEmitter::Flags)((int)a & (int)b);
}
