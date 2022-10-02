/*
 * base.cpp
 *
 *  Created on: 21 Nov 2021
 *      Author: michi
 */

#include "base.h"
#include "helper/PipelineManager.h"
#include "../helper/ResourceManager.h"
#include "../graphics-impl.h"
#include "../lib/image/image.h"
#include "../lib/os/msg.h"
#include "../Config.h"

Texture *tex_white = nullptr;
Texture *tex_black = nullptr;


#ifdef USING_VULKAN

vulkan::Instance *instance = nullptr;
vulkan::DescriptorPool *pool = nullptr;
vulkan::Device *device = nullptr;

void api_init(GLFWwindow* window) {
	instance = vulkan::init({"glfw", "validation", "api=1.2", "rtx?", "verbosity=1"});
	try {
		device = vulkan::Device::create_simple(instance, window, {"graphics", "present", "swapchain", "anisotropy", "compute", "validation"});
	} catch (...) {
		msg_error("warning: no vulkan compute device found. Trying without...");
		device = vulkan::Device::create_simple(instance, window, {"graphics", "present", "swapchain", "anisotropy", "validation"});
	}
	device->create_query_pool(16384);
	pool = new vulkan::DescriptorPool("buffer:1024,sampler:1024", 1024);

	tex_white = new Texture();
	tex_black = new Texture();
	tex_white->write(Image(16, 16, White));
	tex_black->write(Image(16, 16, Black));
}

void api_end() {
	PipelineManager::clear();
	ResourceManager::clear();
	delete pool;
	if (device)
		delete device;
	delete instance;
}

void break_point() {}

#endif

#ifdef USING_OPENGL


namespace nix {
	extern bool allow_separate_vertex_arrays;
	int total_mem();
	int available_mem();
}


void api_init(GLFWwindow* window) {
	nix::allow_separate_vertex_arrays = true;
	nix::init();

	if (nix::total_mem() > 0) {
		msg_write(format("VRAM: %d mb  of  %d mb available", nix::available_mem() / 1024, nix::total_mem() / 1024));
	}

	tex_white = new nix::Texture(16, 16, "rgba:i8");
	tex_black = new nix::Texture(16, 16, "rgba:i8");
	tex_white->write(Image(16, 16, White));
	tex_black->write(Image(16, 16, Black));
}

void api_end() {
}

void break_point() {
	if (config.debug) {
		nix::flush();
	}
}

#endif


