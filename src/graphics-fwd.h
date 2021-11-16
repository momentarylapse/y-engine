/*
 * graphics.h
 *
 *  Created on: Nov 16, 2021
 *      Author: michi
 */

#pragma once


#if HAS_LIB_VULKAN

// Vulkan

#elif HAS_LIB_GL

// OpenGL

	namespace nix {
		class Texture;
		class Shader;
		class VertexBuffer;
		class FrameBuffer;
		class DepthBuffer;
		class CubeMap;
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
	using Buffer = nix::Buffer;
	using UniformBuffer = nix::UniformBuffer;

	using Alpha = nix::Alpha;
	using AlphaMode = nix::AlphaMode;

#endif
