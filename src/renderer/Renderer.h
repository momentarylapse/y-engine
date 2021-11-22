/*
 * Renderer.h
 *
 *  Created on: Jan 4, 2020
 *      Author: michi
 */

#pragma once


class rect;

class RenderPath;


class Renderer {
public:
	Renderer();
	virtual ~Renderer();

	int width, height;
	rect area() const;

	RenderPath *render_path;
	void set_render_path(RenderPath *rp);

	virtual bool start_frame() = 0;
	virtual void draw_frame();
	virtual void end_frame() = 0;


	int ch_render= -1;
	int ch_end = -1;
};
