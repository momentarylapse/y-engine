/*
 * FrameBuffer.h
 *
 *  Created on: Jan 2, 2020
 *      Author: michi
 */

#pragma once

#if HAS_LIB_VULKAN

#include <vulkan/vulkan.h>
#include "../base/base.h"
#include "../base/pointer.h"
#include "Texture.h"

namespace vulkan {

class RenderPass;


class DepthBuffer : public Texture {
public:
	bool with_sampler;

	DepthBuffer(int w, int h, VkFormat format, bool with_sampler);
	DepthBuffer(int w, int h, const string &format, bool with_sampler);
	void __init__(int w, int h, const string &format, bool with_sampler);

	void create(int w, int h, VkFormat format);
};

class FrameBuffer : public Sharable<Empty> {
public:
	FrameBuffer(RenderPass *rp, const Array<Texture*> &attachments);
	~FrameBuffer();

	void __init__(RenderPass *rp, const Array<Texture*> &attachments);
	void __delete__();

	VkFramebuffer frame_buffer;
	int width, height;
	void create(RenderPass *rp, const Array<Texture*> &attachments);
	void destroy();

	shared_array<Texture> attachments;
};

}

#endif

