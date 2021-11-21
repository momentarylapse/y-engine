/*
 * WindowRendererGL.h
 *
 *  Created on: 21 Nov 2021
 *      Author: michi
 */


#pragma once

#include "RendererGL.h"
#include "../graphics-fwd.h"
#ifdef USING_OPENGL
#include "../lib/base/callable.h"

struct GLFWwindow;


class WindowRendererGL : public RendererGL {
public:
	WindowRendererGL(GLFWwindow* win, int w, int h);
	virtual ~WindowRendererGL();


	bool start_frame() override;
	void end_frame() override;

	GLFWwindow* window;

	DepthBuffer* depth_buffer;
	FrameBuffer* frame_buffer;


	FrameBuffer *current_frame_buffer() const;
};

#endif
