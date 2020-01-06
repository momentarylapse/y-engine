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

	DepthBuffer(int w, int h, VkFormat format, bool with_sampler);
	DepthBuffer(int w, int h, const string &format, bool with_sampler);
	void create(int w, int h, VkFormat format);
};

class FrameBuffer {
public:
	FrameBuffer(int w, int h, RenderPass *rp, const Array<VkImageView> &attachments);
	~FrameBuffer();

	VkFramebuffer frame_buffer;
	int width, height;
	void create(int w, int h, RenderPass *rp, const Array<VkImageView> &attachments);
	void destroy();
};

}

#endif /* SRC_LIB_VULKAN_FRAMEBUFFER_H_ */
