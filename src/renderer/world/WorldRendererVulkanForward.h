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
class ShadowRendererVulkan;

class WorldRendererVulkanForward : public WorldRendererVulkan {
public:
	WorldRendererVulkanForward(Renderer *parent, vulkan::Device *device);

	void prepare() override;
	void draw() override;

	void render_into_texture(CommandBuffer *cb, RenderPass *rp, FrameBuffer *fb, Camera *cam, RenderViewDataVK &rvd) override;

	vulkan::Device *device;
};

#endif
