/*
 * RenderPathGL.h
 *
 *  Created on: 07.08.2020
 *      Author: michi
 */

#pragma once

#include "RenderPath.h"
#ifdef USING_OPENGL
#include "../lib/base/pointer.h"


class Material;
class UBOLight;
class GLFWwindow;
class rect;
class Material;

enum class ShaderVariant;

class RendererGL;


class RenderPathGL : public RenderPath {
public:

	shared<FrameBuffer> fb2;
	shared<FrameBuffer> fb3;
	shared<FrameBuffer> fb_shadow;
	shared<FrameBuffer> fb_shadow2;
	shared<Shader> shader_depth;
	shared<Shader> shader_fx;
	Material *material_shadow = nullptr;
	shared<Shader> shader_resolve_multisample;

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
	float sa = 0, sb = 0;


	bool using_view_space = false;

	RenderPathGL(const string &name, Renderer *parent, RenderPathType type);

	virtual void render_into_texture(FrameBuffer *fb, Camera *cam, const rect &target_area) = 0;
	void render_into_cubemap(DepthBuffer *fb, CubeMap *cube, const vector &pos);

	void process_blur(FrameBuffer *source, FrameBuffer *target, float threshold, const complex &axis);
	void process_depth(FrameBuffer *source, FrameBuffer *target, const complex &axis);
	void process(const Array<Texture*> &source, FrameBuffer *target, Shader *shader);
	FrameBuffer* do_post_processing(FrameBuffer *source);
	FrameBuffer* resolve_multisampling(FrameBuffer *source);
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

	FrameBuffer *next_fb(FrameBuffer *cur);
	rect dynamic_fb_area() const;
};

#endif

