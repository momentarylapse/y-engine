/*
 * WindowRendererGL.h
 *
 *  Created on: 21 Nov 2021
 *      Author: michi
 */


#pragma once

#include "Renderer.h"
#include "../graphics-fwd.h"
#ifdef USING_OPENGL
#include "../lib/base/pointer.h"
#include "../lib/base/callable.h"


class WindowRendererGL : public Renderer {
public:
	WindowRendererGL(GLFWwindow* win, int w, int h);
	virtual ~WindowRendererGL();


	bool start_frame() override;
	void end_frame() override;

	GLFWwindow* window;

	DepthBuffer* depth_buffer;
	FrameBuffer* frame_buffers;


	RenderPass *default_render_pass() const;
	FrameBuffer *current_frame_buffer() const;
	CommandBuffer *current_command_buffer() const;
};

#endif
