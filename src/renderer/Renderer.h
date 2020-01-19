/*
 * Renderer.h
 *
 *  Created on: Jan 4, 2020
 *      Author: michi
 */

#ifndef SRC_RENDERER_H_
#define SRC_RENDERER_H_

#include "../lib/vulkan/vulkan.h"

class rect;

class Renderer {
public:
	Renderer();
	virtual ~Renderer();

	vulkan::Semaphore *image_available_semaphore;
	vulkan::Semaphore *render_finished_semaphore;
	vulkan::Fence *in_flight_fence;

	vulkan::CommandBuffer *cb;
	virtual vulkan::RenderPass *default_render_pass() { return nullptr; }

	int width, height;
	rect area() const;

	virtual bool start_frame() { return false; }
	virtual void end_frame() {}

	virtual vulkan::FrameBuffer *current_frame_buffer() { return nullptr; }
	virtual vulkan::DepthBuffer *depth_buffer() { return nullptr; }
};

#endif /* SRC_RENDERER_H_ */
