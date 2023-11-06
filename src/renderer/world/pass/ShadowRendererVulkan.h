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

	void prepare(const RenderParams& params) override;
	void draw(const RenderParams& params) override {}

    void render(vulkan::CommandBuffer *cb, SceneView &parent_scene_view);

    void render_shadow_map(CommandBuffer *cb, FrameBuffer *sfb, float scale, RenderViewDataVK &rvd);

	SceneView scene_view;

	RenderPass *_render_pass = nullptr;

	shared<FrameBuffer> fb[2];
    mat4 proj;
    Material *material;
	RenderViewDataVK rvd[2];

    GeometryRendererVulkan *geo_renderer;

	RenderPass *get_render_pass() override { return _render_pass; };
};

#endif
