/*
 * SwapChain.h
 *
 *  Created on: 03.01.2020
 *      Author: michi
 */

#ifndef SRC_LIB_VULKAN_SWAPCHAIN_H_
#define SRC_LIB_VULKAN_SWAPCHAIN_H_


#include "../base/base.h"
#include "../image/color.h"
#include <vulkan/vulkan.h>
#include "FrameBuffer.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


namespace vulkan {


class FrameBuffer;
class RenderPass;
class Semaphore;

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	Array<VkSurfaceFormatKHR> formats;
	Array<VkPresentModeKHR> present_modes;
};
SwapChainSupportDetails query_swap_chain_support(VkPhysicalDevice device);


class SwapChain {
public:
	Array<VkImage> images;
	VkExtent2D extent;
	VkSwapchainKHR swap_chain;
	VkFormat image_format;
	Array<VkImageView> image_views;
	Array<FrameBuffer*> frame_buffers;

	void cleanup();
	void create();

	void create_image_views();

	void create_frame_buffers(RenderPass *rp, DepthBuffer *db);

	bool present(unsigned int image_index, const Array<Semaphore*> &wait_sem);
	bool aquire_image(unsigned int *image_index, Semaphore *signal_sem);
};

extern SwapChain swap_chain;



VkSurfaceFormatKHR choose_swap_surface_format(const Array<VkSurfaceFormatKHR>& available_formats);
VkPresentModeKHR choose_swap_present_mode(const Array<VkPresentModeKHR> available_present_modes);
VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities);



bool has_stencil_component(VkFormat format);


} /* namespace vulkan */

#endif /* SRC_LIB_VULKAN_SWAPCHAIN_H_ */
