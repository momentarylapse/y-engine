/*
 * WorldRendererVulkan.h
 *
 *  Created on: Nov 18, 2021
 *      Author: michi
 */

#pragma once

#include "WorldRenderer.h"
#ifdef USING_VULKAN
#include "geometry/GeometryRendererVulkan.h"
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
class ShadowRendererVulkan;




class WorldRendererVulkan : public WorldRenderer {
public:
	RenderPass *render_pass_cube = nullptr;

	owned<ShadowRendererVulkan> shadow_renderer;
	owned<GeometryRendererVulkan> geo_renderer;


	VertexBuffer *vb_2d;

	shared<DepthBuffer> depth_cube;
	shared<FrameBuffer> fb_cube;
	shared<CubeMap> cube_map;

	RenderViewDataVK rvd_cube[6];

	void create_more();


	WorldRendererVulkan(const string &name, Renderer *parent, Camera *cam, RenderPathType type);
	virtual ~WorldRendererVulkan();

	virtual void render_into_texture(CommandBuffer *cb, RenderPass *rp, FrameBuffer *fb, Camera *cam, RenderViewDataVK &rvd, const RenderParams& params) = 0;
	void render_into_cubemap(CommandBuffer *cb, CubeMap *cube, const vec3 &pos);

	void prepare_lights(Camera *cam, RenderViewDataVK &rvd);
};

#endif

