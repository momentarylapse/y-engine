//
//  RenderPass.cpp
//  3
//
//  Created by <author> on 06/02/2019.
//
//

#include "RenderPass.h"
#include "helper.h"

#include <iostream>
#include <array>

#if HAS_LIB_VULKAN



namespace vulkan {

// so far, we can only create a "default" render pass with 1 color and 1 depth attachement!
	RenderPass::RenderPass(const Array<VkFormat> &format, bool clear, bool presentable) {
		render_pass = nullptr;

		VkAttachmentLoadOp color_load_op = VK_ATTACHMENT_LOAD_OP_LOAD;
		VkAttachmentLoadOp depth_load_op = VK_ATTACHMENT_LOAD_OP_LOAD;
		if (clear) {
			color_load_op = depth_load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
		}

		// color attachments
		for (int i=0; i<format.num-1; i++) {
			VkAttachmentDescription a = {};
			a.format = format[i];
			a.samples = VK_SAMPLE_COUNT_1_BIT;
			a.loadOp = color_load_op;//VK_ATTACHMENT_LOAD_OP_CLEAR;
			a.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			a.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			a.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			a.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			if (presentable)
				a.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			else
				a.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			attachments.add(a);

			VkAttachmentReference r = {};
			r.attachment = i;
			r.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			color_attachment_refs.add(r);
		}

		{
			VkAttachmentDescription a = {};
			a.format = format.back();
			a.samples = VK_SAMPLE_COUNT_1_BIT;
			a.loadOp = depth_load_op;//VK_ATTACHMENT_LOAD_OP_CLEAR;
			a.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			a.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			a.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			a.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			if (presentable)
				a.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			else
				a.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			attachments.add(a);

			depth_attachment_ref = {};
			depth_attachment_ref.attachment = format.num-1;
			depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		}

		subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = color_attachment_refs.num;
		subpass.pColorAttachments = &color_attachment_refs[0];
		subpass.pDepthStencilAttachment = &depth_attachment_ref;

		dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		for (int i=0; i<format.num - 1; i++)
			clear_color.add(Black);
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
		VkRenderPassCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		info.attachmentCount = attachments.num;
		info.pAttachments = &attachments[0];
		info.subpassCount = 1;
		info.pSubpasses = &subpass;
		info.dependencyCount = 1;
		info.pDependencies = &dependency;

		if (vkCreateRenderPass(device, &info, nullptr, &render_pass) != VK_SUCCESS) {
			throw std::runtime_error("failed to create render pass!");
		}
	}

	void RenderPass::destroy() {
		if (render_pass)
			vkDestroyRenderPass(device, render_pass, nullptr);
		render_pass = nullptr;
	}

	void RenderPass::rebuild() {
		destroy();
		create();
	}

};

#endif
