/*
 * ShadowRendererVulkan.h
 *
 *  Created on: Dec 11, 2022
 *      Author: michi
 */

#pragma once

#include "../../Renderer.h"
#include "../../../graphics-fwd.h"
#ifdef USING_VULKAN
#include "../WorldRendererVulkan.h"
#include "../../../lib/math/mat4.h"

class Camera;
class Material;
class PerformanceMonitor;
struct RenderViewDataVK;
class GeometryRendererVulkan;

class ShadowRendererVulkan : public Renderer {
public:
	ShadowRendererVulkan(Renderer *parent);

	void prepare() override;
	void draw() override {}

    void render(vulkan::CommandBuffer *cb, const mat4 &m);

    void render_shadow_map(CommandBuffer *cb, FrameBuffer *sfb, float scale, RenderViewDataVK &rvd);


	RenderPass *_render_pass = nullptr;
    RenderPass *render_pass() const override { return _render_pass; }

	shared<FrameBuffer> fb[2];
    mat4 proj;
    Material *material;
	RenderViewDataVK rvd[2];

    GeometryRendererVulkan *geo_renderer;
};

#endif
