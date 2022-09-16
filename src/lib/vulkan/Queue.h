/*
 * Queue.h
 *
 *  Created on: Sep 21, 2021
 *      Author: michi
 */

#pragma once

#if HAS_LIB_VULKAN

#include "../base/base.h"
#include "../base/optional.h"

namespace vulkan {

	class CommandBuffer;
	class Semaphore;
	class Fence;

class Queue {
public:
	VkQueue queue;

	bool submit(CommandBuffer *cb, const Array<Semaphore*> &wait_sem, const Array<Semaphore*> &signal_sem, Fence *fence);
	void wait_idle();
};



	struct QueueFamilyIndices {
		base::optional<uint32_t> graphics_family;
		base::optional<uint32_t> present_family;

		bool is_complete();
	};


} /* namespace vulkan */

#endif
