/*
 * graphics.h
 *
 *  Created on: Nov 16, 2021
 *      Author: michi
 */

#pragma once


#if HAS_LIB_VULKAN
	#define USING_VULKAN
#else
	#define USING_OPENGL
#endif


#ifdef USING_VULKAN

// Vulkan

	namespace vulkan {
		class Texture;
		class VolumeTexture;
		class CubeMap;
		class Shader;
		class VertexBuffer;
		class FrameBuffer;
		class DepthBuffer;
		class Buffer;
		class UniformBuffer;
		enum class Alpha;
		enum class AlphaMode;
		class Pipeline;
		class DescriptorSet;
		class RenderPass;
		class CommandBuffer;
		class Semaphore;
		class Fence;
		class SwapChain;
		class DescriptorPool;
		class Device;
	}

	using Texture = vulkan::Texture;
	using Shader = vulkan::Shader;
	using VertexBuffer = vulkan::VertexBuffer;
	using FrameBuffer = vulkan::FrameBuffer;
	using DepthBuffer = vulkan::DepthBuffer;
	using CubeMap = vulkan::CubeMap;
	using VolumeTexture = vulkan::VolumeTexture;
	using Buffer = vulkan::Buffer;
	using UniformBuffer = vulkan::UniformBuffer;
	using Pipeline = vulkan::Pipeline;
	using CommandBuffer = vulkan::CommandBuffer;
	using DescriptorSet = vulkan::DescriptorSet;
	using RenderPass = vulkan::RenderPass;

	using Alpha = vulkan::Alpha;
	using AlphaMode = vulkan::AlphaMode;

#endif
#ifdef USING_OPENGL

// OpenGL

	namespace nix {
		class Texture;
		class Shader;
		class VertexBuffer;
		class FrameBuffer;
		class DepthBuffer;
		class CubeMap;
		class VolumeTexture;
		class Buffer;
		class UniformBuffer;
		enum class Alpha;
		enum class AlphaMode;
	}

	using Texture = nix::Texture;
	using Shader = nix::Shader;
	using VertexBuffer = nix::VertexBuffer;
	using FrameBuffer = nix::FrameBuffer;
	using DepthBuffer = nix::DepthBuffer;
	using CubeMap = nix::CubeMap;
	using VolumeTexture = nix::VolumeTexture;
	using Buffer = nix::Buffer;
	using UniformBuffer = nix::UniformBuffer;

	using Alpha = nix::Alpha;
	using AlphaMode = nix::AlphaMode;

#endif
