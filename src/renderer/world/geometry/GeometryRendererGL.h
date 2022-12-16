/*
 * GeometryRendererGL.h
 *
 *  Created on: Dec 15, 2022
 *      Author: michi
 */

#pragma once

#include "GeometryRenderer.h"
#ifdef USING_OPENGL
#include "../../../lib/math/mat4.h"

class Camera;
class PerformanceMonitor;
class Material;

enum class RenderPathType;
enum class ShaderVariant;

class GeometryRendererGL : public GeometryRenderer {
public:
	GeometryRendererGL(RenderPathType type, Renderer *parent);

	void prepare() override;
	void draw() override {}

	void set_material(Material *m, RenderPathType type, ShaderVariant v);
	void set_material_x(Material *m, Shader *shader);

	void draw_skyboxes();
	void draw_particles();
	void draw_terrains();
	void draw_objects_opaque();
	void draw_objects_transparent();
	void draw_objects_instanced();
	void draw_user_meshes(bool transparent);
	void prepare_instanced_matrices();

	void draw_opaque();
	void draw_transparent();
};

#endif
