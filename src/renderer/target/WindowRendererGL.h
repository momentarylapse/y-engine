/*
 * WindowRendererGL.h
 *
 *  Created on: 21 Nov 2021
 *      Author: michi
 */


#pragma once

#include "TargetRenderer.h"
#ifdef USING_OPENGL

struct GLFWwindow;


class WindowRendererGL : public TargetRenderer {
public:
	WindowRendererGL(GLFWwindow* win, int w, int h);


	bool start_frame() override;
	void end_frame() override;

	void prepare() override;
	void draw() override;

	GLFWwindow* window;

	DepthBuffer* depth_buffer;
	FrameBuffer* frame_buffer;


	FrameBuffer *current_frame_buffer() const override;
	DepthBuffer *current_depth_buffer() const override;
};

#endif
