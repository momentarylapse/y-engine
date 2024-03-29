/*
 * Semaphore.h
 *
 *  Created on: 03.01.2020
 *      Author: michi
 */

#ifndef SRC_LIB_VULKAN_SEMAPHORE_H_
#define SRC_LIB_VULKAN_SEMAPHORE_H_

#if HAS_LIB_VULKAN


#include "../base/base.h"
#include <vulkan/vulkan.h>

namespace vulkan {

class Device;


class Fence {
public:
	Fence(Device *device);
	~Fence();

	VkFence fence;
	VkDevice device;

	void reset();
	void wait();
};

class Semaphore {
public:
	Semaphore(Device *device);
	~Semaphore();

	VkSemaphore semaphore;
	VkDevice device;
};


VkFence fence_handle(Fence *f);
Array<VkSemaphore> extract_semaphores(const Array<Semaphore*> &sem);


} /* namespace vulkan */

#endif

#endif /* SRC_LIB_VULKAN_SEMAPHORE_H_ */
