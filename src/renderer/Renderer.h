/*
 * Renderer.h
 *
 *  Created on: Jan 4, 2020
 *      Author: michi
 */

#pragma once


class rect;

#include "../graphics-fwd.h"
#include "../lib/base/pointer.h"


class Renderer {
public:
	Renderer(const string &name, Renderer *parent = nullptr);
	virtual ~Renderer();

	int width, height;
	rect area() const;

	Renderer *parent = nullptr;
	Renderer *child = nullptr;
	void set_child(Renderer *child);

	virtual void draw() = 0;


	virtual FrameBuffer *current_frame_buffer() const;
#ifdef USING_VULKAN
	virtual RenderPass *default_render_pass() const;
	virtual CommandBuffer *current_command_buffer() const;
#endif


	//int ch_render= -1;
	//int ch_end = -1;
	int channel;
};
