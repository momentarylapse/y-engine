#pragma once

#if HAS_LIB_VULKAN


#include "../base/base.h"
#include "../base/pointer.h"
#include "../os/path.h"
#include <vulkan/vulkan.h>
#include "helper.h"

class Image;

namespace vulkan {

	class Texture : public Sharable<base::Empty> {
	public:
		enum class Type {
			NONE,
			DEFAULT,
			DYNAMIC,
			CUBE,
			DEPTH,
			IMAGE,
			VOLUME,
			MULTISAMPLE,
			RENDERBUFFER,
			ARRAY
		};

		Texture();
		Texture(int w, int h, const string &format);
		~Texture();

		void _load(const Path &filename);
		void write(const Image &image);
		void writex(const void *image, int nx, int ny, int nz, const string &format);
		void read(void* data);


		void set_options(const string &op) const;

		void _destroy();
		void _create_image(const void *data, VkImageType type, VkFormat format, int num_layers, VkSampleCountFlagBits samples, bool allow_mip, bool as_storage, bool cube);
		void _create_sampler() const;


		Type type;
		int width, height, depth;
		int mip_levels, num_layers;
		ImageAndMemory image;

		mutable VkImageView view;
		mutable VkSampler sampler;
		mutable VkCompareOp compare_op;
		mutable VkFilter minfilter, magfilter;
		mutable VkSamplerAddressMode address_mode;

		static xfer<Texture> load(const Path &filename);
	};

	class VolumeTexture : public Texture {
	public:
		VolumeTexture(int nx, int ny, int nz, const string &format);
	};

	class TextureArray : public Texture {
	public:
		TextureArray(int w, int h, int layers, const string &format);
	};

	class StorageTexture : public Texture {
	public:
		StorageTexture(int nx, int ny, int nz, const string &format);
	};

	class CubeMap : public Texture {
	public:
		CubeMap(int size, const string &format);
		void write_side(int side, const Image &image);
	};

	class TextureMultiSample : public Texture {
	public:
		TextureMultiSample(int nx, int ny, int samples, const string &format);
	};
};

#endif

