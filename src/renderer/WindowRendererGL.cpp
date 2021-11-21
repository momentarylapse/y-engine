/*
 * WindowRendererGL.cpp
 *
 *  Created on: 21 Nov 2021
 *      Author: michi
 */

#include "WindowRendererGL.h"
#ifdef USING_OPENGL
#include <GLFW/glfw3.h>
#include "base.h"
#include "../lib/nix/nix.h"
#include "../lib/file/msg.h"
#include "../helper/PerformanceMonitor.h"


WindowRendererGL::WindowRendererGL(GLFWwindow* win, int w, int h) {
	window = win;
	glfwMakeContextCurrent(window);
	//glfwGetFramebufferSize(window, &width, &height);
	width = w;
	height = h;

	frame_buffer = nix::FrameBuffer::DEFAULT;
}

WindowRendererGL::~WindowRendererGL() {
}


FrameBuffer *WindowRendererGL::current_frame_buffer() const {
	return frame_buffer;
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

#endif
