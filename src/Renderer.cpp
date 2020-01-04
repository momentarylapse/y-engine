/*
 * Renderer.cpp
 *
 *  Created on: Jan 4, 2020
 *      Author: michi
 */

#include "Renderer.h"
#include <iostream>

namespace vulkan {
	extern int device_width, device_height;
}

WindowRenderer *WindowRenderer::main_renderer = nullptr;



Renderer::Renderer() {
	image_available_semaphore = new vulkan::Semaphore();
	render_finished_semaphore = new vulkan::Semaphore();
	in_flight_fence = new vulkan::Fence();

	cb = new vulkan::CommandBuffer();
	default_render_pass = nullptr;

	width = height = 0;
}


Renderer::~Renderer() {
	delete render_finished_semaphore;
	delete image_available_semaphore;
	delete in_flight_fence;

	delete cb;
	if (default_render_pass)
		delete default_render_pass;
}





WindowRenderer::WindowRenderer(GLFWwindow *_window) {
	window = _window;


	vulkan::swap_chain.create();
	width = vulkan::swap_chain.extent.width;
	height = vulkan::swap_chain.extent.height;

	default_render_pass = new vulkan::RenderPass({vulkan::swap_chain.image_format, vulkan::swap_chain.depth_buffer->format});
	vulkan::swap_chain.create_frame_buffers(default_render_pass, vulkan::swap_chain.depth_buffer);

	main_renderer = this;
	glfwSetFramebufferSizeCallback(window, framebuffer_resize_callback);
}

WindowRenderer::~WindowRenderer() {
	vulkan::swap_chain.cleanup();
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


	vulkan::swap_chain.rebuild();

	default_render_pass->rebuild();
	vulkan::swap_chain.create_frame_buffers(default_render_pass, vulkan::swap_chain.depth_buffer);

	vulkan::rebuild_pipelines();
}

vulkan::FrameBuffer *WindowRenderer::current_frame_buffer() {
	return vulkan::swap_chain.frame_buffers[image_index];
}

bool WindowRenderer::start_frame() {

	vulkan::target_width = vulkan::device_width;
	vulkan::target_height = vulkan::device_height;

	in_flight_fence->wait();

	if (!vulkan::swap_chain.aquire_image(&image_index, image_available_semaphore)) {
		rebuild_default_stuff();
		return false;
	}
	return true;
}

void WindowRenderer::end_frame() {

	vulkan::queue_submit_command_buffer(cb, {image_available_semaphore}, {render_finished_semaphore}, in_flight_fence);

	if (!vulkan::swap_chain.present(image_index, {render_finished_semaphore}) or framebuffer_resized) {
		framebuffer_resized = false;
		rebuild_default_stuff();
	}
}









TextureRenderer::TextureRenderer(vulkan::Texture *t) {
	tex = t;
	width = tex->width;
	height = tex->height;

	VkExtent2D extent = {(unsigned)width, (unsigned)height};
	depth_buffer = new vulkan::DepthBuffer(extent, VK_FORMAT_D32_SFLOAT);

	default_render_pass = new vulkan::RenderPass({tex->format, depth_buffer->format}, true);
	frame_buffer = new vulkan::FrameBuffer(default_render_pass, {tex->view, depth_buffer->view}, extent);
}

bool TextureRenderer::start_frame() {
	in_flight_fence->wait();
	return true;
}

void TextureRenderer::end_frame() {
	vulkan::queue_submit_command_buffer(cb, {}, {render_finished_semaphore}, in_flight_fence);
}


vulkan::FrameBuffer *TextureRenderer::current_frame_buffer() {
	return frame_buffer;
}

