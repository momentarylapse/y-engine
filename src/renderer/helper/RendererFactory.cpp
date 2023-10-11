/*
 * RendererFactory.cpp
 *
 *  Created on: 11 Oct 2023
 *      Author: michi
 */

#include "RendererFactory.h"
#include "../base.h"
#include "../world/WorldRenderer.h"
#ifdef USING_VULKAN
	#include "../world/WorldRendererVulkan.h"
	#include "../world/WorldRendererVulkanForward.h"
	#include "../world/WorldRendererVulkanRayTracing.h"
	#include "../gui/GuiRendererVulkan.h"
	#include "../post/HDRRendererVulkan.h"
	#include "../post/PostProcessorVulkan.h"
	#include "../target/WindowRendererVulkan.h"
#else
	#include "../world/WorldRendererGL.h"
	#include "../world/WorldRendererGLForward.h"
	#include "../world/WorldRendererGLDeferred.h"
	#include "../gui/GuiRendererGL.h"
	#include "../post/HDRRendererGL.h"
	#include "../post/PostProcessorGL.h"
	#include "../regions/RegionRendererGL.h"
	#include "../target/WindowRendererGL.h"
using RegionRenderer = RegionRendererGL;
#endif
#include <y/EngineData.h>
#include <world/Camera.h>
#include <lib/os/msg.h>
#include <lib/hui_minimal/hui.h>
#include <helper/PerformanceMonitor.h>
#include <Config.h>



string render_graph_str(Renderer *r) {
	string s = PerformanceMonitor::get_name(r->channel);
	if (r->children.num == 1)
		s += " <<< " + render_graph_str(r->children[0]);
	if (r->children.num >= 2) {
		Array<string> ss;
		for (auto c: r->children)
			ss.add(render_graph_str(c));
		s += " <<< (" + implode(ss, ", ") + ")";
	}
	return s;
}

void print_render_chain() {
	msg_write("------------------------------------------");
	msg_write("CHAIN:  " + render_graph_str(engine.window_renderer));
	msg_write("------------------------------------------");
}


TargetRenderer *create_window_renderer(GLFWwindow* window) {
#ifdef USING_VULKAN
	return new WindowRendererVulkan(window, engine.width, engine.height, device);
#else
	return new WindowRendererGL(window, engine.width, engine.height);
#endif
}

Renderer *create_gui_renderer(Renderer *parent) {
#ifdef USING_VULKAN
	return new GuiRendererVulkan(parent);
#else
	return new GuiRendererGL(parent);
#endif
}

RegionRenderer *create_region_renderer(Renderer *parent) {
#ifdef USING_VULKAN
	return new RegionRendererVulkan(parent);
#else
	return new RegionRendererGL(parent);
#endif
}

PostProcessorStage *create_hdr_renderer(PostProcessor *parent) {
#ifdef USING_VULKAN
	return new HDRRendererVulkan(parent);
#else
	return new HDRRendererGL(parent);
#endif
}

PostProcessor *create_post_processor(Renderer *parent) {
#ifdef USING_VULKAN
	return new PostProcessorVulkan(parent);
#else
	return new PostProcessorGL(parent);
#endif
}

WorldRenderer *create_world_renderer(Renderer *parent, Camera *cam) {
#ifdef USING_VULKAN
	if (config.get_str("renderer.path", "forward") == "raytracing")
		return new WorldRendererVulkanRayTracing(parent, device, cam);
	else
		return new WorldRendererVulkanForward(parent, device, cam);
#else
	if (config.get_str("renderer.path", "forward") == "deferred")
		return new WorldRendererGLDeferred(parent, cam);
	else
		return new WorldRendererGLForward(parent, cam);
#endif
}

Renderer *create_render_path(Renderer *parent, Camera *cam) {
	if (config.get_str("renderer.path", "forward") == "direct") {
		engine.world_renderer = create_world_renderer(parent, cam);
		return engine.world_renderer;
	} else {
		engine.post_processor = create_post_processor(parent);
		engine.hdr_renderer = create_hdr_renderer(engine.post_processor);
		engine.world_renderer = create_world_renderer(engine.hdr_renderer, cam);
		//post_processor->set_hdr(hdr_renderer);
		return engine.post_processor;
	}
}

void create_full_renderer(GLFWwindow* window, Camera *cam) {
	try {
		engine.window_renderer = create_window_renderer(window);
		engine.gui_renderer = create_gui_renderer(engine.window_renderer);
		auto region_renderer = create_region_renderer(engine.gui_renderer);
		engine.region_renderer = region_renderer;

		auto p = create_render_path(region_renderer->add_region(rect::ID), cam);
	} catch(Exception &e) {
		hui::ShowError(e.message());
		throw e;
	}
	print_render_chain();
}
