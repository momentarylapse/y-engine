/*
 * Renderer.h
 *
 *  Created on: Jan 4, 2020
 *      Author: michi
 */

#pragma once


class rect;


class Renderer {
public:
	Renderer();
	virtual ~Renderer();

	int width, height;
	rect area() const;

	virtual bool start_frame() { return false; }
	virtual void end_frame() {}
};
