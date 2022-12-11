/*
 * WorldRendererGL.h
 *
 *  Created on: 07.08.2020
 *      Author: michi
 */

#pragma once

#include "WorldRenderer.h"
#ifdef USING_OPENGL


class rect;
class Material;
class Any;

enum class ShaderVariant;

class RendererGL;
class WorldRendererGL;


class WorldRendererGL : public WorldRenderer {
public:
	VertexBuffer *vb_fx = nullptr;
	UniformBuffer *ubo_light = nullptr;

	WorldRendererGL(const string &name, Renderer *parent, RenderPathType type);

	//virtual void render_into_texture(FrameBuffer *fb, Camera *cam) = 0;
	void render_into_cubemap(DepthBuffer *fb, CubeMap *cube, const vec3 &pos);

	void set_material(Material *m, RenderPathType type, ShaderVariant v);

	void draw_skyboxes();
	void draw_particles();
	void draw_terrains(bool allow_material);
	void draw_objects_opaque(bool allow_material);
	void draw_objects_transparent(bool allow_material, RenderPathType t);
	void draw_objects_instanced(bool allow_material);
	void draw_line_meshes(bool allow_material);
	void draw_point_meshes(bool allow_material);
	void prepare_instanced_matrices();
	void prepare_lights();

	void draw_user_mesh(VertexBuffer *vb, Shader *s, const mat4 &m, const Array<Texture*> &tex, const Any &data);
};

#endif

