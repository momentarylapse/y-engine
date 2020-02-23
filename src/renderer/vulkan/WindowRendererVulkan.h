/*
 * WindowRenderer.h
 *
 *  Created on: 05.01.2020
 *      Author: michi
 */

#ifndef SRC_RENDERER_WINDOWRENDERER_H_
#define SRC_RENDERER_WINDOWRENDERER_H_

#if HAS_LIB_VULKAN


#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "../Renderer.h"


class WindowRendererVulkan : public RendererVulkan {
public:
	WindowRendererVulkan(GLFWwindow *window);
	~WindowRendererVulkan() override;

	GLFWwindow *window;
	bool framebuffer_resized = false;

	vulkan::SwapChain *swap_chain;

	vulkan::RenderPass *default_render_pass() override { return swap_chain->default_render_pass; }

	uint32_t image_index = 0;

	void rebuild_default_stuff();

	bool start_frame() override;
	void end_frame() override;

	vulkan::FrameBuffer *current_frame_buffer() override;
	vulkan::DepthBuffer *depth_buffer() override;

	static void framebuffer_resize_callback(GLFWwindow* window, int width, int height);
	static WindowRendererVulkan *main_renderer;
	void on_resize(int width, int height);
};

#endif


#endif /* SRC_RENDERER_WINDOWRENDERER_H_ */
