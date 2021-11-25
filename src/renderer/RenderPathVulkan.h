/*
 * RenderPathVulkan.h
 *
 *  Created on: Nov 18, 2021
 *      Author: michi
 */

#pragma once

#include "RenderPath.h"
#ifdef USING_VULKAN
#include "../lib/base/pointer.h"
#include "../lib/base/callable.h"

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

enum class ShaderVariant;


struct UBO {
	matrix m,v,p;
	color albedo, emission;
	float roughness, metal;
	int num_lights;
};

struct RenderDataVK {
	UniformBuffer* ubo;
	DescriptorSet* dset;
};


class RenderPathVulkan : public RenderPath {
public:


	shared<FrameBuffer> fb2;
	shared<FrameBuffer> fb3;
	shared<Shader> shader_depth;
	shared<Shader> shader_fx;

	shared<FrameBuffer> fb_shadow;
	shared<FrameBuffer> fb_shadow2;
	RenderPass *render_pass_shadow = nullptr;
	//Pipeline *pipeline_shadow = nullptr;
	Material *material_shadow = nullptr;
	shared<Shader> shader_resolve_multisample;
	//Entity3D *shadow_entity = nullptr;
	//Camera *shadow_cam = nullptr;



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
	Array<RenderDataVK> rda_ob;
	Array<RenderDataVK> rda_ob_shadow;


	bool using_view_space = false;

	RenderPathVulkan(const string &name, Renderer *parent, RenderPathType type);
	virtual ~RenderPathVulkan();

	virtual void render_into_texture(FrameBuffer *fb, Camera *cam, const rect &target_area) = 0;
	void render_into_cubemap(DepthBuffer *fb, CubeMap *cube, const vector &pos);


	void process_depth(FrameBuffer *source, FrameBuffer *target, const complex &axis);
	void process(CommandBuffer *cb, const Array<Texture*> &source, FrameBuffer *target, Shader *shader);
	FrameBuffer* do_post_processing(FrameBuffer *source);
	FrameBuffer* resolve_multisampling(FrameBuffer *source);
	void set_material(CommandBuffer *cb, RenderPass *rp, DescriptorSet *dset, Material *m, RenderPathType type, ShaderVariant v);
	void set_textures(DescriptorSet *dset, int i0, int n, const Array<Texture*> &tex);

	void draw_particles();
	void draw_skyboxes(CommandBuffer *cb, Camera *c);
	void draw_terrains(CommandBuffer *cb, RenderPass *rp, UBO &ubo, bool allow_material, Array<RenderDataVK> &rda);
	void draw_objects_opaque(CommandBuffer *cb, RenderPass *rp, UBO &ubo, bool allow_material, Array<RenderDataVK> &rda);
	void draw_objects_transparent(bool allow_material, RenderPathType t);
	void draw_objects_instanced(bool allow_material);
	void prepare_instanced_matrices();
	void prepare_lights(Camera *cam);

	FrameBuffer *next_fb(FrameBuffer *cur);
	rect dynamic_fb_area() const;
};

#endif

