/*
 * graphics-impl.h
 *
 *  Created on: Nov 16, 2021
 *      Author: michi
 */

#pragma once

#include "graphics-fwd.h"


#ifdef USING_VULKAN

// Vulkan

	#include "lib/vulkan/vulkan.h"

#else

// OpenGL

	#include "lib/nix/nix.h"

#endif

