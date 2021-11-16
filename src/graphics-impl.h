/*
 * graphics-impl.h
 *
 *  Created on: Nov 16, 2021
 *      Author: michi
 */

#pragma once


#if HAS_LIB_VULKAN

// Vulkan

	#include "lib/vulkan/vulkan.h"

#elif HAS_LIB_GL

// OpenGL

	#include "lib/nix/nix.h"

#endif

