/*
 * base.h
 *
 *  Created on: 21 Nov 2021
 *      Author: michi
 */


#pragma once

#include "Renderer.h"
#include "../graphics-fwd.h"

struct GLFWwindow;

Context* api_init(GLFWwindow* window);
void api_end();

void break_point();

extern Texture *tex_white;
extern Texture *tex_black;

#ifdef USING_VULKAN
extern vulkan::DescriptorPool *pool;
extern vulkan::Device *device;
#endif
