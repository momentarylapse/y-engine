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

	frame_buffer = nix::FrameBuffer::DEFAULT;
}


FrameBuffer *WindowRendererGL::current_frame_buffer() const {
	return frame_buffer;
}

DepthBuffer *WindowRendererGL::current_depth_buffer() const {
	return depth_buffer;
}


bool WindowRendererGL::start_frame() {
	nix::start_frame_glfw(window);
	//jitter_iterate();
	return true;
}

void WindowRendererGL::end_frame() {
	PerformanceMonitor::begin(ch_end);
	nix::end_frame_glfw(window);
	break_point();
	PerformanceMonitor::end(ch_end);
}

void WindowRendererGL::prepare() {

}

void WindowRendererGL::draw() {

	if (child)
		child->prepare();

	nix::bind_frame_buffer(frame_buffer);

	if (child)
		child->draw();
}

#endif
