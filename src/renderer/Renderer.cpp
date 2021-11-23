/*
 * Renderer.cpp
 *
 *  Created on: Jan 4, 2020
 *      Author: michi
 */

#include "Renderer.h"
#include "RenderPath.h"
#include "../lib/math/rect.h"
#include "../helper/PerformanceMonitor.h"


Renderer::Renderer(const string &name, Renderer *_parent) {
	width = height = 0;
	parent = _parent;
	if (parent) {
		width = parent->width;
		height = parent->height;
	}

	channel = PerformanceMonitor::create_channel(name, parent ? parent->channel : -1);
	//ch_render = PerformanceMonitor::create_channel("render");
	//ch_end = PerformanceMonitor::create_channel("end", ch_render);
}


Renderer::~Renderer() {
}

rect Renderer::area() const {
	return rect(0, width, 0, height);
}

void Renderer::set_child(Renderer *_child) {
	child = _child;
}

FrameBuffer *Renderer::current_frame_buffer() const {
	if (!parent)
		return nullptr;
	return parent->current_frame_buffer();
}

#ifdef USING_VULKAN
RenderPass *Renderer::default_render_pass() const {
	if (!parent)
		return nullptr;
	return parent->default_render_pass();
}

CommandBuffer *Renderer::current_command_buffer() const {
	if (!parent)
		return nullptr;
	return parent->current_command_buffer();
}
#endif
