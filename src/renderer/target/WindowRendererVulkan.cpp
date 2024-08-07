/*
 * WindowRendererVulkan.cpp
 *
 *  Created on: 21 Nov 2021
 *      Author: michi
 */

#include "WindowRendererVulkan.h"
#ifdef USING_VULKAN
#include "../../graphics-impl.h"
#include "../../helper/PerformanceMonitor.h"
#include "../../lib/os/msg.h"
#include "../../lib/math/rect.h"
#include "../../Config.h" // for timing experiment


WindowRendererVulkan::WindowRendererVulkan(GLFWwindow* _window, Device *_device) : TargetRenderer("win") {
	window = _window;
	glfwMakeContextCurrent(window);

	device = _device;


	image_available_semaphore = new vulkan::Semaphore(device);
	render_finished_semaphore = new vulkan::Semaphore(device);


	framebuffer_resized = false;

	_create_swap_chain_and_stuff();
}

WindowRendererVulkan::~WindowRendererVulkan() = default;



void WindowRendererVulkan::_create_swap_chain_and_stuff() {
	swap_chain = new vulkan::SwapChain(window, device);
	auto swap_images = swap_chain->create_textures();
	for (auto t: swap_images)
		wait_for_frame_fences.add(new vulkan::Fence(device));

	for (auto t: swap_images)
		command_buffers.add(device->command_pool->create_command_buffer());

	depth_buffer = swap_chain->create_depth_buffer();
	default_render_pass = swap_chain->create_render_pass(depth_buffer);
	frame_buffers = swap_chain->create_frame_buffers(default_render_pass, depth_buffer);
}


/*func _delete_swap_chain_and_stuff()
	for fb in frame_buffers
		del fb
	del _default_render_pass
	del depth_buffer
	del swap_chain*/

void WindowRendererVulkan::rebuild_default_stuff() {
	msg_write("recreate swap chain");

	device->wait_idle();

	//_delete_swap_chain_and_stuff();
	_create_swap_chain_and_stuff();
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
	device->present_queue.submit(command_buffers[image_index], {image_available_semaphore}, {render_finished_semaphore}, f);

	swap_chain->present(image_index, {render_finished_semaphore});

	device->wait_idle();
	PerformanceMonitor::end(ch_end);
}

RenderParams WindowRendererVulkan::create_params(float aspect_ratio) {
	auto p = RenderParams::into_window(frame_buffers[image_index], aspect_ratio);
	p.command_buffer = command_buffers[image_index];
	p.render_pass = default_render_pass;
	return p;
}

void WindowRendererVulkan::prepare(const RenderParams& params) {

}

void WindowRendererVulkan::draw(const RenderParams& params) {
	PerformanceMonitor::begin(ch_draw);
	auto cb = params.command_buffer;
	auto rp = params.render_pass;
	auto fb = params.frame_buffer;

	cb->begin();
	for (auto c: children)
		c->prepare(params);

	cb->set_viewport({0, (float)swap_chain->width, 0, (float)swap_chain->height});
	cb->begin_render_pass(rp, fb);

	for (auto c: children)
		c->draw(params);

	cb->end_render_pass();
	cb->end();
	PerformanceMonitor::end(ch_draw);
}





#endif
