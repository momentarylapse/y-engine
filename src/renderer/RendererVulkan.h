/*
 * RendererVulkan.h
 *
 *  Created on: 21 Nov 2021
 *      Author: michi
 */

#pragma once

#include "Renderer.h"
#include "../graphics-fwd.h"
#ifdef USING_VULKAN
#include "../lib/base/pointer.h"


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

class RendererVulkan : public Renderer {
public:
	/*RendererVulkan();
	virtual ~RendererVulkan();*/

	vulkan::DescriptorPool* pool = nullptr;

	virtual RenderPass *default_render_pass() const = 0;
	virtual FrameBuffer *current_frame_buffer() const = 0;
	virtual CommandBuffer *current_command_buffer() const = 0;
};

#endif
