#if HAS_LIB_VULKAN

#include "Texture.h"
#include <vulkan/vulkan.h>

#include <cmath>
#include <iostream>

#include "helper.h"
#include "CommandBuffer.h"
#include "../image/image.h"

namespace vulkan {

VkFormat parse_format(const string &s) {
	if (s == "rgba:i8")
		return VK_FORMAT_R8G8B8A8_UNORM;
	if (s == "rgb:i8")
		return VK_FORMAT_R8G8B8_UNORM;
	if (s == "r:i8")
		return VK_FORMAT_R8_UNORM;
	if (s == "rgba:f32")
		return VK_FORMAT_R32G32B32A32_SFLOAT;
	if (s == "rgb:f32")
		return VK_FORMAT_R32G32B32_SFLOAT;
	if (s == "r:f32")
		return VK_FORMAT_R32_SFLOAT;
	std::cerr << "unknown image format: " << s.c_str() << "\n";
	return VK_FORMAT_R8G8B8A8_UNORM;
}

int pixel_size(VkFormat f) {
	if (f == VK_FORMAT_R8G8B8A8_UNORM)
		return 4;
	if (f == VK_FORMAT_R8G8B8_UNORM)
		return 3;
	if (f == VK_FORMAT_R8_UNORM)
		return 1;
	if (f == VK_FORMAT_R32G32B32A32_SFLOAT)
		return 16;
	if (f == VK_FORMAT_R32G32B32_SFLOAT)
		return 12;
	if (f == VK_FORMAT_R32_SFLOAT)
		return 4;
	return 4;
}


Texture::Texture() {
	image = nullptr;
	memory = nullptr;
	sampler = nullptr;
	view = nullptr;
	width = height = depth = 0;
	mip_levels = 0;
}

Texture::~Texture() {
	vkDestroySampler(device, sampler, nullptr);
	vkDestroyImageView(device, view, nullptr);

	vkDestroyImage(device, image, nullptr);
	vkFreeMemory(device, memory, nullptr);
}

void Texture::__init__() {
	new(this) Texture();
}

void Texture::__delete__() {
	this->~Texture();
}

Texture* Texture::load(const string &filename) {
	Texture *t = new Texture();
	t->_load(filename);
	return t;
}


void Texture::_load(const string &filename) {
	Image *im = LoadImage(filename);
	if (!im) {
		throw std::runtime_error("failed to load texture image!");
	}
	override(im);
	delete im;
}

void Texture::override(const Image *im) {
	overridex(im->data.data, im->width, im->height, 1, "rgba:i8");
}

void Texture::overridex(const void *data, int nx, int ny, int nz, const string &format) {
	_create_image(data, nx, ny, nz, parse_format(format));
	_create_view();
	_create_sampler();
}

void Texture::_create_image(const void *image_data, int nx, int ny, int nz, VkFormat image_format) {
	width = nx;
	height = ny;
	depth = nz;
	int ps = pixel_size(image_format);
	VkDeviceSize image_size = width * height * depth * ps;
	mip_levels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;


	VkBuffer staging_buffer;
	VkDeviceMemory staging_memory;
	create_buffer(image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_memory);

	void* data;
	vkMapMemory(device, staging_memory, 0, image_size, 0, &data);
		memcpy(data, image_data, static_cast<size_t>(image_size));
	vkUnmapMemory(device, staging_memory);

	create_image(width, height, depth, mip_levels, image_format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, memory);

	transition_image_layout(image, image_format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mip_levels);
	copy_buffer_to_image(staging_buffer, image, width, height, depth);

	vkDestroyBuffer(device, staging_buffer, nullptr);
	vkFreeMemory(device, staging_memory, nullptr);

	if (depth == 1)
		_generate_mipmaps(image_format);
}

void Texture::_generate_mipmaps(VkFormat image_format) {
	// Check if image format supports linear blitting
	VkFormatProperties fp;
	vkGetPhysicalDeviceFormatProperties(physical_device, image_format, &fp);

	if (!(fp.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
		throw std::runtime_error("texture image format does not support linear blitting!");
	}

	VkCommandBuffer command_buffer = begin_single_time_commands();

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;

	int32_t mipWidth = width;
	int32_t mipHeight = height;

	for (uint32_t i=1; i<mip_levels; i++) {
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(command_buffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		VkImageBlit blit = {};
		blit.srcOffsets[0] = {0, 0, 0};
		blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0] = {0, 0, 0};
		blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		vkCmdBlitImage(command_buffer,
			image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &blit,
			VK_FILTER_LINEAR);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(command_buffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		if (mipWidth > 1) mipWidth /= 2;
		if (mipHeight > 1) mipHeight /= 2;
	}

	barrier.subresourceRange.baseMipLevel = mip_levels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(command_buffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
		0, nullptr,
		0, nullptr,
		1, &barrier);

	end_single_time_commands(command_buffer);
}



void Texture::_create_view() {
	view = create_image_view(image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, mip_levels);
}

void Texture::_create_sampler() {
	VkSamplerCreateInfo si = {};
	si.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	si.magFilter = VK_FILTER_LINEAR;
	si.minFilter = VK_FILTER_LINEAR;
	si.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	si.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	si.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	si.anisotropyEnable = VK_TRUE;
	si.maxAnisotropy = 16;
	si.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	si.unnormalizedCoordinates = VK_FALSE;
	si.compareEnable = VK_FALSE;
	si.compareOp = VK_COMPARE_OP_ALWAYS;
	si.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	si.minLod = 0;
	si.maxLod = static_cast<float>(mip_levels);
	si.mipLodBias = 0;

	if (vkCreateSampler(device, &si, nullptr, &sampler) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture sampler!");
	}
}


};

#endif
