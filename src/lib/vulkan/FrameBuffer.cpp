/*
 * FrameBuffer.cpp
 *
 *  Created on: Jan 2, 2020
 *      Author: michi
 */

#if HAS_LIB_VULKAN


#include "vulkan.h"
#include "FrameBuffer.h"
#include "helper.h"
#include <iostream>

namespace vulkan {

VkFormat parse_format(const string &s);


DepthBuffer::DepthBuffer(int w, int h, VkFormat _format, bool _with_sampler) {
	create(w, h, _format);
}

DepthBuffer::DepthBuffer(int w, int h, const string &format, bool _with_sampler) : DepthBuffer(w, h, parse_format(format), _with_sampler) {}

void DepthBuffer::__init__(int w, int h, const string &format, bool _with_sampler) {
	new(this) DepthBuffer(w, h, format, _with_sampler);
}

void DepthBuffer::create(int w, int h, VkFormat _format) {
	width = w;
	height = h;
	depth = 1;
	mip_levels = 1;
	format = _format;

	auto usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	//if (!with_sampler)
	//	usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	image.create(VK_IMAGE_TYPE_2D, width, height, 1, 1, format, VK_IMAGE_TILING_OPTIMAL, usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, false);
	view = image.create_view(format, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_VIEW_TYPE_2D, 1);

	image.transition_layout(format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);


	//if (with_sampler)
	_create_sampler();
}




FrameBuffer::FrameBuffer(RenderPass *rp, const Array<Texture*> &attachments) {
	frame_buffer = nullptr;
	create(rp, attachments);
}

FrameBuffer::~FrameBuffer() {
	destroy();
}



void FrameBuffer::__init__(RenderPass *rp, const Array<Texture*> &attachments) {
	new(this) FrameBuffer(rp, attachments);
}

void FrameBuffer::__delete__() {
	this->~FrameBuffer();
}

void FrameBuffer::create(RenderPass *rp, const Array<Texture*> &_attachments) {
	attachments.clear();
	for (auto a: _attachments)
		attachments.add(a);
	width = 1;
	height = 1;
	if (attachments.num > 0) {
		width = attachments[0]->width;
		height = attachments[0]->height;
	}

	Array<VkImageView> views;
	for (auto a: _attachments)
		views.add(a->view);

	VkFramebufferCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	info.renderPass = rp->render_pass;
	info.attachmentCount = views.num;
	info.pAttachments = &views[0];
	info.width = width;
	info.height = height;
	info.layers = 1;

	if (vkCreateFramebuffer(default_device->device, &info, nullptr, &frame_buffer) != VK_SUCCESS) {
		throw Exception("failed to create framebuffer!");
	}
}

void FrameBuffer::destroy() {
	if (frame_buffer)
		vkDestroyFramebuffer(default_device->device, frame_buffer, nullptr);
	frame_buffer = nullptr;
}


} /* namespace vulkan */

#endif

