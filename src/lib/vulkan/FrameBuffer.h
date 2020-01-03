/*
 * FrameBuffer.h
 *
 *  Created on: Jan 2, 2020
 *      Author: michi
 */

#ifndef SRC_LIB_VULKAN_FRAMEBUFFER_H_
#define SRC_LIB_VULKAN_FRAMEBUFFER_H_


#include <vulkan/vulkan.h>
#include "../base/base.h"

namespace vulkan {

class RenderPass;


class DepthBuffer {
public:
	VkFormat format;
	VkImage image;
	VkDeviceMemory memory;
	VkImageView view;

	DepthBuffer(VkExtent2D extent, VkFormat format);
	void create(VkExtent2D extent, VkFormat format);
	void destroy();
};

class FrameBuffer {
public:
	FrameBuffer(RenderPass *rp, const Array<VkImageView> &attachments, VkExtent2D extent);
	~FrameBuffer();

	VkFramebuffer frame_buffer;
	VkExtent2D extent;
	void create(RenderPass *rp, const Array<VkImageView> &attachments, VkExtent2D extent);
	void destroy();
};

}

#endif /* SRC_LIB_VULKAN_FRAMEBUFFER_H_ */
