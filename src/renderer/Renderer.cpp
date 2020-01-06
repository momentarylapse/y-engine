/*
 * Renderer.cpp
 *
 *  Created on: Jan 4, 2020
 *      Author: michi
 */

#include "Renderer.h"
#include "../lib/math/rect.h"


Renderer::Renderer() {
	image_available_semaphore = new vulkan::Semaphore();
	render_finished_semaphore = new vulkan::Semaphore();
	in_flight_fence = new vulkan::Fence();

	cb = new vulkan::CommandBuffer();

	width = height = 0;
}


Renderer::~Renderer() {
	delete render_finished_semaphore;
	delete image_available_semaphore;
	delete in_flight_fence;

	delete cb;
}

rect Renderer::area() const {
	return rect(0, width, 0, height);
}



