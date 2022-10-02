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

	vulkan::Device *device;

    vulkan::StorageTexture *offscreen_image;
    vulkan::Texture *offscreen_image2;
    vulkan::ComputePipeline *pipeline;
    vulkan::DescriptorSet *dset;
    vulkan::UniformBuffer *buffer_vertices;
    vulkan::UniformBuffer *buffer_materials;


    shared<Shader> shader_out;
    GraphicsPipeline* pipeline_out;
    DescriptorSet *dset_out;
    VertexBuffer *vb_2d;
};

#endif
