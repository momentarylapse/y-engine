/*
 * WindowRenderer.cpp
 *
 *  Created on: 05.01.2020
 *      Author: michi
 */

#include "WindowRenderer.h"
#include <iostream>


namespace vulkan {
	extern int device_width, device_height; // TODO: make obsolete
}

WindowRenderer *WindowRenderer::main_renderer = nullptr;




WindowRenderer::WindowRenderer(GLFWwindow *_window) {
	window = _window;


	swap_chain = new vulkan::SwapChain();
	width = swap_chain->extent.width;
	height = swap_chain->extent.height;

	default_render_pass = swap_chain->default_render_pass;

	main_renderer = this;
	glfwSetFramebufferSizeCallback(window, framebuffer_resize_callback);
}

WindowRenderer::~WindowRenderer() {
	delete swap_chain;
}


void WindowRenderer::framebuffer_resize_callback(GLFWwindow* window, int width, int height) {
	main_renderer->on_resize(width, height);
}

void WindowRenderer::on_resize(int w, int h) {
	width = w;
	height = h;
	vulkan::device_width = width;
	vulkan::device_height = height;
	framebuffer_resized = true;
}

void WindowRenderer::rebuild_default_stuff() {

	std::cout << "recreate swap chain" << "\n";

	vulkan::wait_device_idle();


	swap_chain->rebuild();

	vulkan::rebuild_pipelines();
}

vulkan::FrameBuffer *WindowRenderer::current_frame_buffer() {
	return swap_chain->frame_buffers[image_index];
}

bool WindowRenderer::start_frame() {
	in_flight_fence->wait();

	if (!swap_chain->aquire_image(&image_index, image_available_semaphore)) {
		rebuild_default_stuff();
		return false;
	}
	return true;
}

void WindowRenderer::end_frame() {

	vulkan::queue_submit_command_buffer(cb, {image_available_semaphore}, {render_finished_semaphore}, in_flight_fence);

	if (!swap_chain->present(image_index, {render_finished_semaphore}) or framebuffer_resized) {
		framebuffer_resized = false;
		rebuild_default_stuff();
	}
}




