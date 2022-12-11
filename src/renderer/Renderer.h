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

class rect;
class color;

rect dynamicly_scaled_area(FrameBuffer *fb);
rect dynamicly_scaled_source();


class Renderer {
public:
	Renderer(const string &name, Renderer *parent = nullptr);
	virtual ~Renderer();

	int width, height;
	rect area() const;

	Renderer *parent = nullptr;
	Array<Renderer*> children;
	void add_child(Renderer *child);

	// (vulkan: BEFORE/OUTSIDE a render pass)
	// can render into separate targets
	virtual void prepare();

	// assume, parent has already bound the frame buffer
	// (vulkan: INSIDE an already started render pass)
	// just draw into that
	virtual void draw() = 0;

	virtual color background() const;


	virtual FrameBuffer *frame_buffer() const;
	virtual DepthBuffer *depth_buffer() const;
#ifdef USING_VULKAN
	virtual RenderPass *render_pass() const;
	virtual CommandBuffer *command_buffer() const;
#endif

	virtual bool forwarding_into_window() const;
	bool rendering_into_window() const;


	//int ch_render= -1;
	//int ch_end = -1;
	int channel;
};
