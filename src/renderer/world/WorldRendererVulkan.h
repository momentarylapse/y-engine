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
#include "../../lib/math/vec3.h"
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
class Entity;
class Any;

enum class ShaderVariant;


struct UBO {
	// matrix
	mat4 m,v,p;
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
	mat4 m,v,p;
};

struct RenderDataFxVK {
	UniformBuffer *ubo;
	DescriptorSet *dset;
	VertexBuffer *vb;
};


class WorldRendererVulkan : public WorldRenderer {
public:

	RenderPass *render_pass_shadow = nullptr;
	RenderPass *render_pass_cube = nullptr;


	VertexBuffer *vb_2d;

	shared<DepthBuffer> depth_cube;
	shared<FrameBuffer> fb_cube;
	shared<CubeMap> cube_map;

	struct RenderViewDataVK {
		UniformBuffer *ubo_light = nullptr;
		Array<RenderDataVK> rda_tr;
		Array<RenderDataVK> rda_ob;
		Array<RenderDataVK> rda_ob_trans;
		Array<RenderDataVK> rda_sky;
		Array<RenderDataFxVK> rda_fx;
	};
	RenderViewDataVK rvd_def;
	RenderViewDataVK rvd_cube[6];
	RenderViewDataVK rvd_shadow1, rvd_shadow2;

	Pipeline *pipeline_fx = nullptr;


	WorldRendererVulkan(const string &name, Renderer *parent, RenderPathType type);
	virtual ~WorldRendererVulkan();

	virtual void render_shadow_map(CommandBuffer *cb, FrameBuffer *sfb, float scale, RenderViewDataVK &rvd) = 0;
	virtual void render_into_texture(CommandBuffer *cb, RenderPass *rp, FrameBuffer *fb, Camera *cam, RenderViewDataVK &rvd) = 0;
	void render_into_cubemap(CommandBuffer *cb, CubeMap *cube, const vec3 &pos);


	void set_material(CommandBuffer *cb, RenderPass *rp, DescriptorSet *dset, Material *m, RenderPathType type, ShaderVariant v);
	void set_textures(DescriptorSet *dset, int i0, int n, const Array<Texture*> &tex);

	void draw_particles(CommandBuffer *cb, RenderPass *rp, Camera *cam, RenderViewDataVK &rvd);
	void draw_skyboxes(CommandBuffer *cb, RenderPass *rp, Camera *cam, float aspect, RenderViewDataVK &rvd);
	void draw_terrains(CommandBuffer *cb, RenderPass *rp, UBO &ubo, bool allow_material, RenderViewDataVK &rvd);
	void draw_objects_opaque(CommandBuffer *cb, RenderPass *rp, UBO &ubo, bool allow_material, RenderViewDataVK &rvd);
	void draw_objects_transparent(CommandBuffer *cb, RenderPass *rp, UBO &ubo, RenderViewDataVK &rvd);
	void draw_objects_instanced(bool allow_material);
	void prepare_instanced_matrices();
	void prepare_lights(Camera *cam, RenderViewDataVK &rvd);


	void draw_user_mesh(VertexBuffer *vb, Shader *s, const mat4 &m, const Array<Texture*> &tex, const Any &data);
};

#endif

