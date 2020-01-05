/*
 * FrameBuffer.cpp
 *
 *  Created on: Jan 2, 2020
 *      Author: michi
 */

#include "vulkan.h"
#include "FrameBuffer.h"
#include "helper.h"
#include <iostream>

namespace vulkan {


DepthBuffer::DepthBuffer(VkExtent2D extent, VkFormat _format, bool _with_sampler) {
	with_sampler = _with_sampler;
	create(extent, _format);
}

void DepthBuffer::create(VkExtent2D extent, VkFormat _format) {
	format = _format;

	auto usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	if (!with_sampler)
		usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	create_image(extent.width, extent.height, 1, 1, format, VK_IMAGE_TILING_OPTIMAL, usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, memory);
	view = create_image_view(image, format, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

	transition_image_layout(image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);


	if (with_sampler)
		_create_sampler();
}




FrameBuffer::FrameBuffer(RenderPass *rp, const Array<VkImageView> &attachments, VkExtent2D extent) {
	frame_buffer = nullptr;
	create(rp, attachments, extent);
}

FrameBuffer::~FrameBuffer() {
	destroy();
}


void FrameBuffer::create(RenderPass *rp, const Array<VkImageView> &attachments, VkExtent2D _extent) {
	extent = _extent;

	VkFramebufferCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	info.renderPass = rp->render_pass;
	info.attachmentCount = attachments.num;
	info.pAttachments = &attachments[0];
	info.width = extent.width;
	info.height = extent.height;
	info.layers = 1;

	if (vkCreateFramebuffer(device, &info, nullptr, &frame_buffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to create framebuffer!");
	}
}

void FrameBuffer::destroy() {
	if (frame_buffer)
		vkDestroyFramebuffer(device, frame_buffer, nullptr);
	frame_buffer = nullptr;
}


} /* namespace vulkan */
