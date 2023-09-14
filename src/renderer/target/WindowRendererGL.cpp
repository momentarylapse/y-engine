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

WindowRendererGL::WindowRendererGL(GLFWwindow* win, int w, int h) : TargetRenderer("win") {
	window = win;
	glfwMakeContextCurrent(window);
	//glfwGetFramebufferSize(window, &width, &height);
	width = w;
	height = h;

	_frame_buffer = context->default_framebuffer;
}


FrameBuffer *WindowRendererGL::frame_buffer() const {
	return _frame_buffer;
}

DepthBuffer *WindowRendererGL::depth_buffer() const {
	return _depth_buffer;
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

void WindowRendererGL::prepare() {

}

void WindowRendererGL::draw() {

	for (auto c: children)
		c->prepare();

	bool prev_srgb = nix::get_srgb();
	nix::set_srgb(true);
	nix::bind_frame_buffer(_frame_buffer);

	for (auto c: children)
		c->draw();

	nix::set_srgb(prev_srgb);
}

#endif
