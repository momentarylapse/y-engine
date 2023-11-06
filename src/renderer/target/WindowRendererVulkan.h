/*
 * WindowRendererVulkan.h
 *
 *  Created on: 21 Nov 2021
 *      Author: michi
 */

#pragma once

#include "TargetRenderer.h"
#ifdef USING_VULKAN

struct GLFWwindow;


using Semaphore = vulkan::Semaphore;
using Fence = vulkan::Fence;
using SwapChain = vulkan::SwapChain;
using RenderPass = vulkan::RenderPass;
using Device = vulkan::Device;

class WindowRendererVulkan : public TargetRenderer {
public:
	WindowRendererVulkan(GLFWwindow* win, Device *device);
	virtual ~WindowRendererVulkan();


	bool start_frame() override;
	void end_frame() override;

	void prepare(const RenderParams& params) override;
	void draw(const RenderParams& params) override;

	RenderParams create_params(float aspect_ratio);

	GLFWwindow* window;

	Fence* in_flight_fence;
	Array<Fence*> wait_for_frame_fences;
	Semaphore *image_available_semaphore, *render_finished_semaphore;

	Array<CommandBuffer*> _command_buffers;
	//var cb: vulkan::CommandBuffer*

	Device *device;
	SwapChain *swap_chain;
	RenderPass* _default_render_pass;
	DepthBuffer* _depth_buffer;
	Array<FrameBuffer*> _frame_buffers;
	int image_index;
	bool framebuffer_resized;

	void _create_swap_chain_and_stuff();
	void rebuild_default_stuff();

	RenderPass *get_render_pass() override { return _default_render_pass; };
};

#endif
