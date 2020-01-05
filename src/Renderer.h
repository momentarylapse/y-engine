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
	vulkan::RenderPass *default_render_pass;

	int width, height;

	virtual bool start_frame() { return false; }
	virtual void end_frame() {}

	virtual vulkan::FrameBuffer *current_frame_buffer() { return nullptr; }
};

class WindowRenderer : public Renderer {
public:
	WindowRenderer(GLFWwindow *window);
	~WindowRenderer() override;

	GLFWwindow *window;
	bool framebuffer_resized = false;

	vulkan::SwapChain *swap_chain;

	uint32_t image_index = 0;

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
	~TextureRenderer() override;

	vulkan::Texture *tex;

	vulkan::DepthBuffer *depth_buffer;
	vulkan::FrameBuffer *frame_buffer;

	bool start_frame() override;
	void end_frame() override;

	vulkan::FrameBuffer *current_frame_buffer() override;
};

class GBufferRenderer : public Renderer {
public:
	GBufferRenderer();
	~GBufferRenderer() override;

	vulkan::Texture *tex_color;
	vulkan::Texture *tex_pos;
	vulkan::Texture *tex_normal;

	vulkan::DepthBuffer *depth_buffer;
	vulkan::FrameBuffer *frame_buffer;

	vulkan::Shader *shader_into_gbuf;
	vulkan::Pipeline *pipeline;

	bool start_frame() override;
	void end_frame() override;

	vulkan::FrameBuffer *current_frame_buffer() override;
};

#endif /* SRC_RENDERER_H_ */
