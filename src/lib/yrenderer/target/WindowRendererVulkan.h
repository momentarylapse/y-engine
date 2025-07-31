/*
 * WindowRendererVulkan.h
 *
 *  Created on: 21 Nov 2021
 *      Author: michi
 */

#pragma once

#include "TargetRenderer.h"
#ifdef USING_VULKAN

#include <lib/vulkan/vulkan.h>

struct GLFWwindow;


using Semaphore = vulkan::Semaphore;
using Fence = vulkan::Fence;
using SwapChain = vulkan::SwapChain;
using RenderPass = vulkan::RenderPass;
using Device = vulkan::Device;

namespace yrenderer {

class SurfaceRendererVulkan : public TargetRenderer {
public:
	SurfaceRendererVulkan(Context* ctx, const string& name);
	~SurfaceRendererVulkan() override;


	bool start_frame();
	void end_frame(const RenderParams& params);

	void prepare(const RenderParams& params) override;
	void draw(const RenderParams& params) override;

	virtual void create_swap_chain() = 0;

	RenderParams create_params(float aspect_ratio);

	Fence* in_flight_fence;
	Array<Fence*> wait_for_frame_fences;
	Semaphore *image_available_semaphore = nullptr;
	Semaphore *render_finished_semaphore = nullptr;

	Array<ygfx::CommandBuffer*> command_buffers;

	Device *device;
	SwapChain *swap_chain = nullptr;
	Array<ygfx::Texture*> swap_images;
	RenderPass* default_render_pass = nullptr;
	ygfx::DepthBuffer* depth_buffer = nullptr;
	Array<ygfx::FrameBuffer*> frame_buffers;
	int image_index = 0;
	bool framebuffer_resized = false;

	void _create_swap_chain_and_stuff();
	void rebuild_default_stuff();
};

#ifdef HAS_LIB_GLFW
class WindowRendererVulkan : public SurfaceRendererVulkan {
public:
	WindowRendererVulkan(Context* ctx, GLFWwindow* win);

	void create_swap_chain() override;

	GLFWwindow* window;

	static xfer<WindowRendererVulkan> create(Context* ctx, GLFWwindow* win);
};
#endif

class HeadlessSurfaceRendererVulkan : public SurfaceRendererVulkan {
public:
	HeadlessSurfaceRendererVulkan(Context* ctx, int width, int height);

	void create_swap_chain() override;

	int width, height;

	static xfer<HeadlessSurfaceRendererVulkan> create(Context* ctx, int width, int height);
};

}

#endif
