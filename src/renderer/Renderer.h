/*
 * Renderer.h
 *
 *  Created on: Jan 4, 2020
 *      Author: michi
 */

#pragma once

#if HAS_LIB_VULKAN
#include "../lib/vulkan/vulkan.h"
#endif

class rect;


class Renderer {
public:
	Renderer();
	virtual ~Renderer();

	int width, height;
	rect area() const;

	virtual bool start_frame() { return false; }
	virtual void end_frame() {}
};

#if HAS_LIB_VULKAN
class RendererVulkan : public Renderer {
public:
	RendererVulkan();
	virtual ~RendererVulkan();

	vulkan::Semaphore *image_available_semaphore;
	vulkan::Semaphore *render_finished_semaphore;
	vulkan::Fence *in_flight_fence;

	vulkan::CommandBuffer *cb;
	virtual vulkan::RenderPass *default_render_pass() { return nullptr; }

	virtual vulkan::FrameBuffer *current_frame_buffer() { return nullptr; }
	virtual vulkan::DepthBuffer *depth_buffer() { return nullptr; }
};
#endif
