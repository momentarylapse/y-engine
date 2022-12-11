/*
 * WorldRendererVulkanForward.h
 *
 *  Created on: Nov 18, 2021
 *      Author: michi
 */

#pragma once

#include "WorldRendererVulkan.h"
#ifdef USING_VULKAN

class Camera;
class PerformanceMonitor;
struct UBO;
class ShadowPassVulkan;

class WorldRendererVulkanForward : public WorldRendererVulkan {
public:
	WorldRendererVulkanForward(Renderer *parent, vulkan::Device *device);

	void prepare() override;
	void draw() override;

	void render_into_texture(CommandBuffer *cb, RenderPass *rp, FrameBuffer *fb, Camera *cam, RenderViewDataVK &rvd) override;
	void render_shadow_map(CommandBuffer *cb, FrameBuffer *sfb, float scale, RenderViewDataVK &rvd) override;

	vulkan::Device *device;

	ShadowPassVulkan *shadow_pass = nullptr;
};

#endif
