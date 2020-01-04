/*
 * Renderer.h
 *
 *  Created on: Jan 4, 2020
 *      Author: michi
 */

#ifndef SRC_RENDERER_H_
#define SRC_RENDERER_H_

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "lib/vulkan/vulkan.h"

class Renderer {
public:
	Renderer();
	virtual ~Renderer();

	vulkan::Semaphore *image_available_semaphore;
	vulkan::Semaphore *render_finished_semaphore;
	vulkan::Fence *in_flight_fence;

	vulkan::CommandBuffer *cb;

	virtual bool start_frame() { return false; }
	virtual void end_frame() {}

	virtual vulkan::FrameBuffer *current_frame_buffer() { return nullptr; }
};

class WindowRenderer : public Renderer {
public:
	WindowRenderer(GLFWwindow *window);
	virtual ~WindowRenderer();

	GLFWwindow *window;
	bool framebuffer_resized = false;

	uint32_t image_index = 0;

	vulkan::DepthBuffer *depth_buffer;
	vulkan::RenderPass *default_render_pass;

	void rebuild_default_stuff();

	bool start_frame() override;
	void end_frame() override;

	vulkan::FrameBuffer *current_frame_buffer() override;

	static void framebuffer_resize_callback(GLFWwindow* window, int width, int height);
	static WindowRenderer *main_renderer;
	void on_resize(int width, int height);
};

class TextureRenderer : public Renderer {
public:
	TextureRenderer(vulkan::Texture *tex);

	vulkan::Texture *tex;

	vulkan::DepthBuffer *depth_buffer;
	vulkan::FrameBuffer *frame_buffer;
	vulkan::RenderPass *render_pass;

	bool start_frame() override;
	void end_frame() override;

	vulkan::FrameBuffer *current_frame_buffer() override;
};

#endif /* SRC_RENDERER_H_ */