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
#include "geometry/RenderViewData.h"
#include <lib/base/pointer.h>
#include <lib/base/callable.h>
#include <lib/math/vec3.h>
#include <lib/math/rect.h>

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
class ShadowRenderer;
class CubeMapSource;




class WorldRendererVulkan : public WorldRenderer {
public:
	owned<ShadowRenderer> shadow_renderer;
	owned<GeometryRenderer> geo_renderer;

	void create_more();


	WorldRendererVulkan(const string &name, Camera *cam, RenderPathType type);
	~WorldRendererVulkan() override;

	virtual void render_into_texture(Camera *cam, RenderViewData &rvd, const RenderParams& params) = 0;
	void render_into_cubemap(CubeMapSource& source, const RenderParams& params);

	void prepare_lights(Camera *cam, RenderViewData &rvd);
};

#endif

