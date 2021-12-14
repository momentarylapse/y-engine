/*
 * WorldRendererGL.h
 *
 *  Created on: 07.08.2020
 *      Author: michi
 */

#pragma once

#include "WorldRenderer.h"
#ifdef USING_OPENGL
#include "../../lib/base/pointer.h"


class Material;
class UBOLight;
class GLFWwindow;
class rect;
class Material;
class Any;

enum class ShaderVariant;

class RendererGL;


class WorldRendererGL : public WorldRenderer {
public:

	shared<FrameBuffer> fb_shadow;
	shared<FrameBuffer> fb_shadow2;
	shared<Shader> shader_fx;
	Material *material_shadow = nullptr;

	Array<UBOLight> lights;
	UniformBuffer *ubo_light;
	VertexBuffer *vb_2d;

	shared<DepthBuffer> depth_cube;
	shared<FrameBuffer> fb_cube;
	shared<CubeMap> cube_map;

	//Camera *shadow_cam;
	matrix shadow_proj;
	int shadow_index;

	float shadow_box_size;
	int shadow_resolution;


	bool using_view_space = false;

	WorldRendererGL(const string &name, Renderer *parent, RenderPathType type);

	virtual void render_into_texture(FrameBuffer *fb, Camera *cam) = 0;
	void render_into_cubemap(DepthBuffer *fb, CubeMap *cube, const vector &pos);

	void set_material(Material *m, RenderPathType type, ShaderVariant v);
	void set_textures(const Array<Texture*> &tex);

	void draw_particles();
	void draw_skyboxes(Camera *c);
	void draw_terrains(bool allow_material);
	void draw_objects_opaque(bool allow_material);
	void draw_objects_transparent(bool allow_material, RenderPathType t);
	void draw_objects_instanced(bool allow_material);
	void prepare_instanced_matrices();
	void prepare_lights(Camera *cam);

	void draw_user_mesh(VertexBuffer *vb, Shader *s, const Array<Texture*> &tex, const Any &data);
};

#endif

