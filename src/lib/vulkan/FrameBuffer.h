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
#include "Texture.h"

namespace vulkan {

class RenderPass;


class DepthBuffer : public Texture {
public:
	bool with_sampler;

	DepthBuffer(VkExtent2D extent, VkFormat format, bool with_sampler);
	void create(VkExtent2D extent, VkFormat format);
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
