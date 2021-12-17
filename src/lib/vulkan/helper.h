#pragma once

#if HAS_LIB_VULKAN

#include "../base/base.h"
#include <vulkan/vulkan.h>

namespace vulkan{

	class FrameBuffer;

	struct ImageAndMemory {
		VkImage image = nullptr;
		VkDeviceMemory memory = nullptr;

		void create(VkImageType type, uint32_t width, uint32_t height, uint32_t depth, uint32_t mip_levels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, bool cube);
		void _destroy();

		VkImageView create_view(VkFormat format, VkImageAspectFlags aspect_flags, VkImageViewType type, uint32_t mip_levels) const;
		void transition_layout(VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout, uint32_t mip_levels) const;
	};

	//void create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& buffer_memory);
	void copy_buffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size);
	void copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t depth);

	bool has_stencil_component(VkFormat format);
};

#endif
