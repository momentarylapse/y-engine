//
//  RenderPass.cpp
//  3
//
//  Created by <author> on 06/02/2019.
//
//

#include "RenderPass.h"
#include "helper.h"

#include <array>

#if HAS_LIB_VULKAN



namespace vulkan {

// so far, we can only create a "default" render pass with 1 color and 1 depth attachement!
	RenderPass::RenderPass(const Array<VkFormat> &format, bool clear, bool presentable) {
		VkAttachmentLoadOp color_load_op = VK_ATTACHMENT_LOAD_OP_LOAD;
		VkAttachmentLoadOp depth_load_op = VK_ATTACHMENT_LOAD_OP_LOAD;
		if (clear) {
			color_load_op = depth_load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
		}

		color_attachment = {};
		color_attachment.format = format[0];
		color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		color_attachment.loadOp = color_load_op;//VK_ATTACHMENT_LOAD_OP_CLEAR;
		color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		if (presentable)
			color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		else
			color_attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		depth_attachment = {};
		depth_attachment.format = format[1];
		depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depth_attachment.loadOp = depth_load_op;//VK_ATTACHMENT_LOAD_OP_CLEAR;
		depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		if (presentable)
			depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		else
			depth_attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		color_attachment_ref = {};
		color_attachment_ref.attachment = 0;
		color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		depth_attachment_ref = {};
		depth_attachment_ref.attachment = 1;
		depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &color_attachment_ref;
		subpass.pDepthStencilAttachment = &depth_attachment_ref;

		dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		clear_color = Black;
		clear_z = 1.0f;
		clear_stencil = 0;

		create();
	}

	RenderPass::~RenderPass() {
		destroy();
	}


	void RenderPass::__init__(const Array<VkFormat> &format, bool clear) {
		new(this) RenderPass(format, clear, true);
	}

	void RenderPass::__delete__() {
		this->~RenderPass();
	}

	void RenderPass::create() {
		std::array<VkAttachmentDescription, 2> attachments = {color_attachment, depth_attachment};
		VkRenderPassCreateInfo render_pass_info = {};
		render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		render_pass_info.attachmentCount = static_cast<uint32_t>(attachments.size());
		render_pass_info.pAttachments = attachments.data();
		render_pass_info.subpassCount = 1;
		render_pass_info.pSubpasses = &subpass;
		render_pass_info.dependencyCount = 1;
		render_pass_info.pDependencies = &dependency;

		if (vkCreateRenderPass(device, &render_pass_info, nullptr, &render_pass) != VK_SUCCESS) {
			throw std::runtime_error("failed to create render pass!");
		}
	}

	void RenderPass::destroy() {
		vkDestroyRenderPass(device, render_pass, nullptr);
	}

	void RenderPass::rebuild() {
		destroy();
		create();
	}

};

#endif
