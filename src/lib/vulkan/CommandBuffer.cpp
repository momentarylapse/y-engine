//
//  CommandBuffer.cpp
//  3
//
//  Created by <author> on 06/02/2019.
//
//

#if HAS_LIB_VULKAN


#include "CommandBuffer.h"
#include "helper.h"
#include "VertexBuffer.h"
#include "Pipeline.h"
#include "RenderPass.h"
#include "Shader.h"
#include "FrameBuffer.h"
#include "../image/color.h"
#include "../math/rect.h"
#include <array>

#include <iostream>

namespace vulkan{

	VkCommandPool command_pool;
	extern VkQueue graphics_queue;


void create_command_pool() {
	QueueFamilyIndices queue_family_indices = find_queue_families(physical_device);

	VkCommandPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_info.queueFamilyIndex = queue_family_indices.graphics_family.value();

	if (vkCreateCommandPool(device, &pool_info, nullptr, &command_pool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create command pool!");
	}
}

void destroy_command_pool() {
	vkDestroyCommandPool(device, command_pool, nullptr);
}


VkCommandBuffer begin_single_time_commands() {
	VkCommandBufferAllocateInfo ai = {};
	ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	ai.commandPool = command_pool;
	ai.commandBufferCount = 1;

	VkCommandBuffer command_buffer;
	vkAllocateCommandBuffers(device, &ai, &command_buffer);

	VkCommandBufferBeginInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(command_buffer, &info);

	return command_buffer;
}

void end_single_time_commands(VkCommandBuffer command_buffer) {
	vkEndCommandBuffer(command_buffer);

	VkSubmitInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	info.commandBufferCount = 1;
	info.pCommandBuffers = &command_buffer;

	vkQueueSubmit(graphics_queue, 1, &info, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphics_queue);

	vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
}

CommandBuffer::CommandBuffer() {
	_create();
}

CommandBuffer::~CommandBuffer() {
	_destroy();
}

void CommandBuffer::__init__() {
	new(this) CommandBuffer();
}

void CommandBuffer::__delete__() {
	this->~CommandBuffer();
}

void CommandBuffer::_create() {

	VkCommandBufferAllocateInfo ai = {};
	ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	ai.commandPool = command_pool;
	ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	ai.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(device, &ai, &buffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}
}

void CommandBuffer::_destroy() {
	vkFreeCommandBuffers(device, command_pool, 1, &buffer);
}


//VkDescriptorSet current_set;

void CommandBuffer::set_pipeline(Pipeline *pl) {
	vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pl->pipeline);
	current_pipeline = pl;
}
void CommandBuffer::bind_descriptor_set(int index, DescriptorSet *dset) {
	vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, current_pipeline->layout, index, 1, &dset->descriptor_set, 0, nullptr);
}
void CommandBuffer::push_constant(int offset, int size, void *data) {
	auto stage_flags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	vkCmdPushConstants(buffer, current_pipeline->layout, stage_flags, offset, size, data);
}

void CommandBuffer::draw(VertexBuffer *vb) {
	VkDeviceSize offsets[] = {0};
	vkCmdBindVertexBuffers(buffer, 0, 1, &vb->vertex_buffer, offsets);

	if (vb->index_buffer) {
		vkCmdBindIndexBuffer(buffer, vb->index_buffer, 0, VK_INDEX_TYPE_UINT16);
		vkCmdDrawIndexed(buffer, vb->output_count, 1, 0, 0, 0);
	} else {
		vkCmdDraw(buffer, vb->output_count, 1, 0, 0);
	}
}

void CommandBuffer::begin() {
	VkCommandBufferBeginInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	if (vkBeginCommandBuffer(buffer, &info) != VK_SUCCESS) {
		throw std::runtime_error("failed to begin recording command buffer!");
	}
}

void CommandBuffer::begin_render_pass(RenderPass *rp, FrameBuffer *fb) {
	std::array<VkClearValue, 2> cv = {};
	memcpy((void*)&cv[0].color, &rp->clear_color, sizeof(color));
	cv[1].depthStencil = {rp->clear_z, rp->clear_stencil};

	VkRenderPassBeginInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	info.renderPass = rp->render_pass;
	info.framebuffer = fb->frame_buffer;
	info.renderArea.offset = {0, 0};
	info.renderArea.extent = fb->extent;swap_chain.extent;
	info.clearValueCount = static_cast<uint32_t>(cv.size());
	info.pClearValues = cv.data();

	vkCmdBeginRenderPass(buffer, &info, VK_SUBPASS_CONTENTS_INLINE);
}

void CommandBuffer::scissor(const rect &r) {
	VkRect2D scissor = {(int)r.x1, (int)r.y1, (unsigned int)r.width(), (unsigned int)r.height()};
	vkCmdSetScissor(buffer, 0, 1, &scissor);
}


void CommandBuffer::end_render_pass() {
	vkCmdEndRenderPass(buffer);
}

void CommandBuffer::end() {
	if (vkEndCommandBuffer(buffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer!");
	}
}


};

#endif
