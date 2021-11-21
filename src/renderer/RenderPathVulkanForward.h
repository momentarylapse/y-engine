/*
 * RenderPathVulkanForward.h
 *
 *  Created on: Nov 18, 2021
 *      Author: michi
 */

#pragma once

#include "RenderPathVulkan.h"
#ifdef USING_VULKAN

class Camera;
class PerformanceMonitor;

class RenderPathVulkanForward : public RenderPathVulkan {
public:

	RenderPathVulkanForward(WindowRendererVulkan *r);
	void draw() override;

	void render_into_texture(FrameBuffer *fb, Camera *cam, const rect &target_area) override;
	void draw_world(bool allow_material);
	void render_shadow_map(FrameBuffer *sfb, float scale);
};

#endif
