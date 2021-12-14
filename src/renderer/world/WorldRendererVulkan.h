/*
 * WorldRendererVulkan.h
 *
 *  Created on: Nov 18, 2021
 *      Author: michi
 */

#pragma once

#include "WorldRenderer.h"
#ifdef USING_VULKAN
#include "../../lib/base/pointer.h"
#include "../../lib/base/callable.h"
#include "../../lib/math/vector.h"
#include "../../lib/math/rect.h"

namespace vulkan {
	class Instance;
	class SwapChain;
	class Fence;
	class Semaphore;
	class RenderPass;
	class DescriptorPool;
	class CommandBuffer;
}
using Semaphore = vulkan::Semaphore;
using Fence = vulkan::Fence;
using SwapChain = vulkan::SwapChain;
using RenderPass = vulkan::RenderPass;

class Material;
class UBOLight;
class GLFWwindow;
class rect;
class Material;
class Entity3D;
class Any;

enum class ShaderVariant;


struct UBO {
	// matrix
	matrix m,v,p;
	// material
	color albedo, emission;
	float roughness, metal;
	int dummy[2];
	int num_lights;
	int shadow_index;
	int dummy2[2];
};

struct RenderDataVK {
	UniformBuffer* ubo;
	DescriptorSet* dset;
};

struct UBOFx {
	matrix m,v,p;
};

struct VertexFx {
	vector pos;
	color col;
	float u, v;
};

struct RenderDataFxVK {
	UniformBuffer *ubo;
	DescriptorSet *dset;
	VertexBuffer *vb;
};


class WorldRendererVulkan : public WorldRenderer {
public:


	shared<Shader> shader_fx;

	shared<FrameBuffer> fb_shadow;
	shared<FrameBuffer> fb_shadow2;
	RenderPass *render_pass_shadow = nullptr;
	//Pipeline *pipeline_shadow = nullptr;
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


	Array<RenderDataVK> rda_tr;
	Array<RenderDataVK> rda_tr_shadow;
	Array<RenderDataVK> rda_tr_shadow2;
	Array<RenderDataVK> rda_ob;
	Array<RenderDataVK> rda_ob_shadow;
	Array<RenderDataVK> rda_ob_shadow2;
	Array<RenderDataVK> rda_sky;

	Array<RenderDataFxVK> rda_fx;
	Pipeline *pipeline_fx = nullptr;


	bool using_view_space = false;

	WorldRendererVulkan(const string &name, Renderer *parent, RenderPathType type);
	virtual ~WorldRendererVulkan();

	virtual void render_into_texture(FrameBuffer *fb, Camera *cam, const rect &target_area) = 0;
	void render_into_cubemap(DepthBuffer *fb, CubeMap *cube, const vector &pos);


	void set_material(CommandBuffer *cb, RenderPass *rp, DescriptorSet *dset, Material *m, RenderPathType type, ShaderVariant v);
	void set_textures(DescriptorSet *dset, int i0, int n, const Array<Texture*> &tex);

	void draw_particles(CommandBuffer *cb, RenderPass *rp);
	void draw_skyboxes(CommandBuffer *cb, Camera *c);
	void draw_terrains(CommandBuffer *cb, RenderPass *rp, UBO &ubo, bool allow_material, Array<RenderDataVK> &rda);
	void draw_objects_opaque(CommandBuffer *cb, RenderPass *rp, UBO &ubo, bool allow_material, Array<RenderDataVK> &rda);
	void draw_objects_transparent(bool allow_material, RenderPathType t);
	void draw_objects_instanced(bool allow_material);
	void prepare_instanced_matrices();
	void prepare_lights(Camera *cam);


	void draw_user_mesh(VertexBuffer *vb, Shader *s, const Array<Texture*> &tex, const Any &data);
};

#endif

