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

WindowRendererVulkan::~WindowRendererVulkan() {
}



void WindowRendererVulkan::_create_swap_chain_and_stuff() {
	swap_chain = new vulkan::SwapChain(window, device);
	auto swap_images = swap_chain->create_textures();
	for (auto t: swap_images)
		wait_for_frame_fences.add(new vulkan::Fence(device));

	for (auto t: swap_images)
		_command_buffers.add(device->command_pool->create_command_buffer());

	_depth_buffer = swap_chain->create_depth_buffer();
	_default_render_pass = swap_chain->create_render_pass(_depth_buffer);
	_frame_buffers = swap_chain->create_frame_buffers(_default_render_pass, _depth_buffer);
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
	//PerformanceMonitor::begin(ch_end);
	auto f = wait_for_frame_fences[image_index];
	device->present_queue.submit(_command_buffers[image_index], {image_available_semaphore}, {render_finished_semaphore}, f);

	swap_chain->present(image_index, {render_finished_semaphore});

	device->wait_idle();
	//PerformanceMonitor::end(ch_end);

	static int frame = 0;
	frame ++;
	if ((frame%100 == 0) and (config.debug_level >= 2)) {
		auto tt = device->get_timestamps(0, 3);
		//msg_write(ia2s(tt));
		//msg_write(f2s(device->device_properties.limits.timestampPeriod, 9));
		msg_write("vulkan timing:");
		msg_write(f2s(device->physical_device_properties.limits.timestampPeriod * (tt[1] - tt[0]) * 0.000001f, 3));
		msg_write(f2s(device->physical_device_properties.limits.timestampPeriod * (tt[2] - tt[0]) * 0.000001f, 3));
	}
}

RenderParams WindowRendererVulkan::create_params(float aspect_ratio) {
	auto p = RenderParams::into_window(_frame_buffers[image_index], aspect_ratio);
	p.command_buffer = _command_buffers[image_index];
	p.render_pass = _default_render_pass;
	return p;
}

void WindowRendererVulkan::prepare(const RenderParams& params) {

}

void WindowRendererVulkan::draw(const RenderParams& params) {
	auto cb = params.command_buffer;
	auto rp = params.render_pass;
	auto fb = params.frame_buffer;

	cb->begin();
	for (auto c: children)
		c->prepare(params);

	cb->set_viewport({0, (float)swap_chain->width, 0, (float)swap_chain->height});
	//rp->clear_color = {White};
	cb->begin_render_pass(rp, fb);

	for (auto c: children)
		c->draw(params);

	cb->end_render_pass();
	cb->end();
}





#endif
