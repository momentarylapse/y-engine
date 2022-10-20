/*
 * WorldRendererVulkanRayTracing.h
 *
 *  Created on: Sep 23, 2022
 *      Author: michi
 */

#pragma once

#include "WorldRendererVulkan.h"
#ifdef USING_VULKAN

class Camera;

class WorldRendererVulkanRayTracing : public WorldRendererVulkan {
public:
	WorldRendererVulkanRayTracing(Renderer *parent, vulkan::Device *device);

	void prepare() override;
	void draw() override;

	void render_into_texture(CommandBuffer *cb, RenderPass *rp, FrameBuffer *fb, Camera *cam, RenderViewDataVK &rvd) override;
	void render_shadow_map(CommandBuffer *cb, FrameBuffer *sfb, float scale, RenderViewDataVK &rvd) override {}

	enum class Mode {
		NONE,
		COMPUTE,
		RTX
	} mode = Mode::NONE;

	vulkan::Device *device;

	vulkan::StorageTexture *offscreen_image;
	vulkan::Texture *offscreen_image2;

	struct MeshDescription {
		mat4 matrix;
		color albedo;
		color emission;
		int64 address_vertices;
		int64 address_indices;
		int num_triangles;
		int _a, _b, _c;
	};

	struct PushConst {
		mat4 iview;
		color background;
		int num_trias;
		int num_lights;
		int num_meshes;
		int _a;
	} pc;

	vulkan::UniformBuffer *buffer_meshes;

	struct ComputeModeData {
		vulkan::DescriptorPool *pool;
		vulkan::DescriptorSet *dset;
		vulkan::ComputePipeline *pipeline;
	} compute;

	struct RtxModeData {
		vulkan::DescriptorPool *pool;
		vulkan::DescriptorSet *dset;
		vulkan::RayPipeline *pipeline;
		vulkan::AccelerationStructure *tlas = nullptr;
		Array<vulkan::AccelerationStructure*> blas;
		vulkan::UniformBuffer *buffer_cam;
	} rtx;


	shared<Shader> shader_out;
	GraphicsPipeline* pipeline_out;
	DescriptorSet *dset_out;
	VertexBuffer *vb_2d;

	Entity *dummy_cam_entity;
	Camera *dummy_cam;
};

#endif
