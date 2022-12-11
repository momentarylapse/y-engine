/*
 * ShadowPassVulkan.h
 *
 *  Created on: Dec 11, 2022
 *      Author: michi
 */

#pragma once

#include "../../Renderer.h"
#include "../../../graphics-fwd.h"
#ifdef USING_VULKAN
#include "../../../lib/math/mat4.h"

class Camera;
class PerformanceMonitor;
struct RenderViewDataVK;

class ShadowPassVulkan : public Renderer {
public:
	ShadowPassVulkan(Renderer *parent, vulkan::Texture *tex, vulkan::DepthBuffer *depth);

    void set(const mat4 &shadow_proj, float scale, RenderViewDataVK *rvd);

	void prepare() override;
	void draw() override;

	RenderPass *_render_pass = nullptr;
    RenderPass *render_pass() const override { return _render_pass; }

    float scale;
    mat4 shadow_proj;
    RenderViewDataVK *rvd;
};

#endif
