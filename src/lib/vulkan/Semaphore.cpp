/*
 * Semaphore.cpp
 *
 *  Created on: 03.01.2020
 *      Author: michi
 */

#include "Semaphore.h"
#include "vulkan.h"

namespace vulkan {


Fence::Fence() {
	VkFenceCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	if (vkCreateFence(device, &info, nullptr, &fence) != VK_SUCCESS) {
		throw std::runtime_error("failed to create fence");
	}
}

Fence::~Fence() {
}

void Fence::__init__() {
	new(this) Fence;
}

void Fence::__delete__() {
	this->~Fence();
}

void Fence::reset() {
	vkResetFences(device, 1, &fence);
}

void Fence::wait() {
	vkWaitForFences(device, 1, &fence, VK_TRUE, std::numeric_limits<uint64_t>::max());
}



Semaphore::Semaphore() {
	VkSemaphoreCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	if (vkCreateSemaphore(device, &info, nullptr, &semaphore) != VK_SUCCESS) {
		throw std::runtime_error("failed to create semaphore");
	}
}

Semaphore::~Semaphore() {
	vkDestroySemaphore(device, semaphore, nullptr);
}

void Semaphore::__init__() {
	new(this) Semaphore;
}

void Semaphore::__delete__() {
	this->~Semaphore();
}

} /* namespace vulkan */
