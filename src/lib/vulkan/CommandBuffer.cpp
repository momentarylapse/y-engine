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
	uint32_t image_index;


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

	VkCommandBufferBeginInfo bi = {};
	bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(command_buffer, &bi);

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
	buffers.resize(swap_chain.images.num);

	VkCommandBufferAllocateInfo ai = {};
	ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	ai.commandPool = command_pool;
	ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	ai.commandBufferCount = (uint32_t)buffers.num;

	if (vkAllocateCommandBuffers(device, &ai, &buffers[0]) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}
}

void CommandBuffer::_destroy() {
	vkFreeCommandBuffers(device, command_pool, buffers.num, &buffers[0]);
}


//VkDescriptorSet current_set;

void CommandBuffer::set_pipeline(Pipeline *pl) {
	vkCmdBindPipeline(current, VK_PIPELINE_BIND_POINT_GRAPHICS, pl->pipeline);
	current_pipeline = pl;
}
void CommandBuffer::bind_descriptor_set(int index, DescriptorSet *dset) {
	vkCmdBindDescriptorSets(current, VK_PIPELINE_BIND_POINT_GRAPHICS, current_pipeline->layout, index, 1, &dset->descriptor_sets[image_index], 0, nullptr);
}
void CommandBuffer::push_constant(int offset, int size, void *data) {
	auto stage_flags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	vkCmdPushConstants(current, current_pipeline->layout, stage_flags, offset, size, data);
}

void CommandBuffer::draw(VertexBuffer *vb) {
	VkBuffer vertexBuffers[] = {vb->vertex_buffer};
	VkDeviceSize offsets[] = {0};
	vkCmdBindVertexBuffers(current, 0, 1, vertexBuffers, offsets);

	if (vb->index_buffer) {
		vkCmdBindIndexBuffer(current, vb->index_buffer, 0, VK_INDEX_TYPE_UINT16);
		vkCmdDrawIndexed(current, vb->output_count, 1, 0, 0, 0);
	} else {
		vkCmdDraw(current, vb->output_count, 1, 0, 0);
	}
}

void CommandBuffer::begin() {
	VkCommandBufferBeginInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	current = buffers[image_index];
	if (vkBeginCommandBuffer(current, &info) != VK_SUCCESS) {
		throw std::runtime_error("failed to begin recording command buffer!");
	}
}

void CommandBuffer::begin_render_pass(RenderPass *rp) {
	std::array<VkClearValue, 2> cv = {};
	memcpy((void*)&cv[0].color, &rp->clear_color, sizeof(color));
	cv[1].depthStencil = {rp->clear_z, rp->clear_stencil};

	VkRenderPassBeginInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	info.renderPass = rp->render_pass;
	info.framebuffer = swap_chain.framebuffers[image_index].frame_buffer;
	info.renderArea.offset = {0, 0};
	info.renderArea.extent = swap_chain.extent;
	info.clearValueCount = static_cast<uint32_t>(cv.size());
	info.pClearValues = cv.data();

	vkCmdBeginRenderPass(current, &info, VK_SUBPASS_CONTENTS_INLINE);
}

void CommandBuffer::scissor(const rect &r) {
	VkRect2D scissor = {(int)r.x1, (int)r.y1, (unsigned int)r.width(), (unsigned int)r.height()};
	vkCmdSetScissor(current, 0, 1, &scissor);
}


void CommandBuffer::end_render_pass() {
	vkCmdEndRenderPass(current);
}

void CommandBuffer::end() {
	if (vkEndCommandBuffer(current) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer!");
	}
}


};

#endif
