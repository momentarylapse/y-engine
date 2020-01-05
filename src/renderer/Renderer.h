/*
 * Renderer.h
 *
 *  Created on: Jan 4, 2020
 *      Author: michi
 */

#ifndef SRC_RENDERER_H_
#define SRC_RENDERER_H_

#include "../lib/vulkan/vulkan.h"

class Renderer {
public:
	Renderer();
	virtual ~Renderer();

	vulkan::Semaphore *image_available_semaphore;
	vulkan::Semaphore *render_finished_semaphore;
	vulkan::Fence *in_flight_fence;

	vulkan::CommandBuffer *cb;
	vulkan::RenderPass *default_render_pass;

	int width, height;

	virtual bool start_frame() { return false; }
	virtual void end_frame() {}

	virtual vulkan::FrameBuffer *current_frame_buffer() { return nullptr; }
};

#endif /* SRC_RENDERER_H_ */
