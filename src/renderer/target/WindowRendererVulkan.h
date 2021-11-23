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

class WindowRendererVulkan : public TargetRenderer {
public:
	WindowRendererVulkan(GLFWwindow* win, int w, int h);
	virtual ~WindowRendererVulkan();


	bool start_frame() override;
	void end_frame() override;

	void prepare() override;
	void draw() override;

	GLFWwindow* window;

	Fence* in_flight_fence;
	Array<Fence*> wait_for_frame_fences;
	Semaphore *image_available_semaphore, *render_finished_semaphore;

	Array<CommandBuffer*> command_buffers;
	//var cb: vulkan::CommandBuffer*

	SwapChain *swap_chain;
	RenderPass* _default_render_pass;
	DepthBuffer* depth_buffer;
	Array<FrameBuffer*> frame_buffers;
	int image_index;
	bool framebuffer_resized;

	void _create_swap_chain_and_stuff();
	void rebuild_default_stuff();


	RenderPass *default_render_pass() const override;
	FrameBuffer *current_frame_buffer() const override;
	CommandBuffer *current_command_buffer() const override;
};

#endif
