/*
 * WindowRendererGL.cpp
 *
 *  Created on: 21 Nov 2021
 *      Author: michi
 */

#include "WindowRendererGL.h"
#ifdef USING_OPENGL
#include <GLFW/glfw3.h>
#include "../base.h"
#include "../../lib/nix/nix.h"
#include "../../helper/PerformanceMonitor.h"

WindowRendererGL::WindowRendererGL(GLFWwindow* win) : TargetRenderer("win") {
	window = win;
	glfwMakeContextCurrent(window);
	//glfwGetFramebufferSize(window, &width, &height);

	_frame_buffer = context->default_framebuffer;
}


bool WindowRendererGL::start_frame() {
	nix::start_frame_glfw(context, window);
	//jitter_iterate();
	return true;
}

void WindowRendererGL::end_frame() {
	PerformanceMonitor::begin(ch_end);
	nix::end_frame_glfw();
	break_point();
	PerformanceMonitor::end(ch_end);
}


RenderParams WindowRendererGL::create_params(float aspect_ratio) {
	return RenderParams::into_window(_frame_buffer, aspect_ratio);
}

void WindowRendererGL::prepare(const RenderParams& params) {

}

void WindowRendererGL::draw(const RenderParams& params) {
	PerformanceMonitor::begin(ch_draw);
	auto sub_params = RenderParams::into_window(_frame_buffer, params.desired_aspect_ratio);
	for (auto c: children)
		c->prepare(sub_params);

	bool prev_srgb = nix::get_srgb();
	nix::set_srgb(true);
	nix::bind_frame_buffer(_frame_buffer);

	for (auto c: children)
		c->draw(sub_params);

	nix::set_srgb(prev_srgb);
	PerformanceMonitor::end(ch_draw);
}

#endif
