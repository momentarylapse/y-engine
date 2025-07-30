/*
 * base.h
 *
 *  Created on: 21 Nov 2021
 *      Author: michi
 */


#pragma once

#include "Renderer.h"
#include <lib/ygraphics/graphics-fwd.h>

struct GLFWwindow;

namespace xhui {
class Painter;
}

class ResourceManager;

namespace yrenderer {

struct Context {

	ygfx::Context* context;

	ResourceManager* resource_manager = nullptr;

	ygfx::Texture* tex_white;
	ygfx::Texture* tex_black;

	Array<int> gpu_timestamp_queries;

#ifdef USING_VULKAN
	vulkan::Instance* instance = nullptr;
	vulkan::DescriptorPool* pool;
	vulkan::Device* device;
#endif

	void _create_default_textures();

	void reset_gpu_timestamp_queries();

	void gpu_timestamp(const RenderParams& params, int channel);
	void gpu_timestamp_begin(const RenderParams& params, int channel);
	void gpu_timestamp_end(const RenderParams& params, int channel);
	Array<float> gpu_read_timestamps();

	void gpu_flush();
};

Context* api_init(GLFWwindow* window);
#ifdef USING_VULKAN
Context* api_init_external(vulkan::Instance* instance, vulkan::Device* device);
#endif
Context* api_init_xhui(xhui::Painter* p);
void api_end(Context* ctx);

static constexpr int MAX_TIMESTAMP_QUERIES = 4096;


}
