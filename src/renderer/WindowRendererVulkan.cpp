/*
 * WindowRendererVulkan.cpp
 *
 *  Created on: 21 Nov 2021
 *      Author: michi
 */

#include "WindowRendererVulkan.h"
#ifdef USING_VULKAN
#include "../graphics-impl.h"
#include "../helper/PerformanceMonitor.h"
#include "../lib/file/msg.h"


WindowRendererVulkan::WindowRendererVulkan(GLFWwindow* win, int w, int h) {
	window = win;
	glfwMakeContextCurrent(window);
	//glfwGetFramebufferSize(window, &width, &height);
	width = w;
	height = h;


	image_available_semaphore = new vulkan::Semaphore();
	render_finished_semaphore = new vulkan::Semaphore();


	framebuffer_resized = false;
	pool = new vulkan::DescriptorPool("buffer:1024,sampler:1024", 1024);

	_create_swap_chain_and_stuff();
}

WindowRendererVulkan::~WindowRendererVulkan() {
}



void WindowRendererVulkan::_create_swap_chain_and_stuff() {
	swap_chain = new vulkan::SwapChain(window);
	auto swap_images = swap_chain->create_textures();
	for (auto t: swap_images)
		wait_for_frame_fences.add(new vulkan::Fence());

	for (auto t: swap_images)
		command_buffers.add(new CommandBuffer());

	depth_buffer = swap_chain->create_depth_buffer();
	_default_render_pass = swap_chain->create_render_pass(depth_buffer);
	frame_buffers = swap_chain->create_frame_buffers(_default_render_pass, depth_buffer);
	width = swap_chain->width;
	height = swap_chain->height;
}


/*func _delete_swap_chain_and_stuff()
	for fb in frame_buffers
		del fb
	del _default_render_pass
	del depth_buffer
	del swap_chain*/

void WindowRendererVulkan::rebuild_default_stuff() {
	msg_write("recreate swap chain");

	vulkan::default_device->wait_idle();

	//_delete_swap_chain_and_stuff();
	_create_swap_chain_and_stuff();
}



RenderPass* WindowRendererVulkan::default_render_pass() const {
	return _default_render_pass;
}

FrameBuffer* WindowRendererVulkan::current_frame_buffer() const {
	return frame_buffers[image_index];
}

CommandBuffer* WindowRendererVulkan::current_command_buffer() const {
	return command_buffers[image_index];
}


bool WindowRendererVulkan::start_frame() {

	if (!swap_chain->acquire_image(&image_index, image_available_semaphore)) {
		rebuild_default_stuff();
		return false;
	}

	auto f = wait_for_frame_fences[image_index];
	f->wait();
	f->reset();

	return true;
}

void WindowRendererVulkan::end_frame() {
	PerformanceMonitor::begin(ch_end);
	auto f = wait_for_frame_fences[image_index];
	vulkan::default_device->present_queue.submit(command_buffers[image_index], {image_available_semaphore}, {render_finished_semaphore}, f);

	swap_chain->present(image_index, {render_finished_semaphore});

	vulkan::default_device->wait_idle();
	PerformanceMonitor::end(ch_end);
}





#endif
