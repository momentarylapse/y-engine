/*
 * SwapChain.cpp
 *
 *  Created on: 03.01.2020
 *      Author: michi
 */

#include "SwapChain.h"
#include "vulkan.h"
#include "helper.h"

#include <iostream>

namespace vulkan {



extern int device_width, device_height;




VkSurfaceFormatKHR choose_swap_surface_format(const Array<VkSurfaceFormatKHR>& available_formats) {
	if (available_formats.num == 1 and available_formats[0].format == VK_FORMAT_UNDEFINED) {
		return {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
	}

	for (const auto& format: available_formats) {
		if (format.format == VK_FORMAT_B8G8R8A8_UNORM and format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return format;
		}
	}

	return available_formats[0];
}

VkPresentModeKHR choose_swap_present_mode(const Array<VkPresentModeKHR> available_present_modes) {
	VkPresentModeKHR best_mode = VK_PRESENT_MODE_FIFO_KHR;

	for (const auto& mode : available_present_modes) {
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return mode;
		} else if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
			best_mode = mode;
		}
	}

	return best_mode;
}

VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities) {
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	} else {

		VkExtent2D actual_extent = {(unsigned)device_width, (unsigned)device_height};

		actual_extent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actual_extent.width));
		actual_extent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actual_extent.height));

		return actual_extent;
	}
}


void SwapChain::create_frame_buffers(RenderPass *render_pass, DepthBuffer *depth_buffer) {
	frame_buffers.resize(image_views.num);

	for (size_t i=0; i<image_views.num; i++) {
		frame_buffers[i] = new FrameBuffer(render_pass, {image_views[i], depth_buffer->view}, extent);
	}
}


SwapChainSupportDetails query_swap_chain_support(VkPhysicalDevice device) {
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

	uint32_t format_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);

	if (format_count != 0) {
		details.formats.resize(format_count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, &details.formats[0]);
	}

	uint32_t present_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, nullptr);

	if (present_mode_count != 0) {
		details.present_modes.resize(present_mode_count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, &details.present_modes[0]);
	}

	return details;
}



void SwapChain::create() {
	SwapChainSupportDetails swap_chain_support = query_swap_chain_support(physical_device);

	VkSurfaceFormatKHR surface_format = choose_swap_surface_format(swap_chain_support.formats);
	VkPresentModeKHR present_mode = choose_swap_present_mode(swap_chain_support.present_modes);
	extent = choose_swap_extent(swap_chain_support.capabilities);

	uint32_t image_count = swap_chain_support.capabilities.minImageCount + 1;
	if (swap_chain_support.capabilities.maxImageCount > 0 and image_count > swap_chain_support.capabilities.maxImageCount) {
		image_count = swap_chain_support.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR info = {};
	info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	info.surface = surface;

	info.minImageCount = image_count;
	info.imageFormat = surface_format.format;
	info.imageColorSpace = surface_format.colorSpace;
	info.imageExtent = extent;
	info.imageArrayLayers = 1;
	info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indices = find_queue_families(physical_device);
	uint32_t queue_family_indices[] = {indices.graphics_family.value(), indices.present_family.value()};

	if (indices.graphics_family != indices.present_family) {
		info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		info.queueFamilyIndexCount = 2;
		info.pQueueFamilyIndices = queue_family_indices;
	} else {
		info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	info.preTransform = swap_chain_support.capabilities.currentTransform;
	info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	info.presentMode = present_mode;
	info.clipped = VK_TRUE;

	if (vkCreateSwapchainKHR(device, &info, nullptr, &swap_chain) != VK_SUCCESS) {
		throw std::runtime_error("failed to create swap chain!");
	}

	vkGetSwapchainImagesKHR(device, swap_chain, &image_count, nullptr);
	images.resize(image_count);
	vkGetSwapchainImagesKHR(device, swap_chain, &image_count, &images[0]);

	image_format = surface_format.format;
}





void SwapChain::create_image_views() {
	image_views.resize(images.num);

	for (uint32_t i=0; i<images.num; i++) {
		image_views[i] = create_image_view(images[i], image_format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
	}
}







void SwapChain::cleanup() {

	for (auto frame_buffer: frame_buffers) {
		frame_buffer->destroy();
	}

	default_render_pass->destroy();

	for (auto image_view: image_views) {
		vkDestroyImageView(device, image_view, nullptr);
	}

	vkDestroySwapchainKHR(device, swap_chain, nullptr);
}


bool SwapChain::present(unsigned int image_index, const Array<Semaphore*> &wait_sem) {

	auto wait_semaphores = extract_semaphores(wait_sem);

	std::cout << "-present-   wait sem \n";
	VkPresentInfoKHR present_info = {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = wait_semaphores.num;
	present_info.pWaitSemaphores = &wait_semaphores[0];
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &swap_chain;
	present_info.pImageIndices = &image_index;

	VkResult result = vkQueuePresentKHR(present_queue, &present_info);

	if (result == VK_ERROR_OUT_OF_DATE_KHR or result == VK_SUBOPTIMAL_KHR) {
		return false;
	} else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image!");
	}
	return true;
}

bool SwapChain::aquire_image(unsigned int *image_index, Semaphore *signal_sem) {

	std::cout << "-aquire image-   wait sem image " << signal_sem << "\n";
	VkResult result = vkAcquireNextImageKHR(device, swap_chain, std::numeric_limits<uint64_t>::max(), signal_sem->semaphore, VK_NULL_HANDLE, image_index);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		return false;
	} else if (result != VK_SUCCESS and result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swap chain image!");
	}
	return true;
}

} /* namespace vulkan */
