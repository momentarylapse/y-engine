/*
 * Renderer.cpp
 *
 *  Created on: Jan 4, 2020
 *      Author: michi
 */

#include "Renderer.h"
#include "../lib/math/rect.h"


Renderer::Renderer() {
	width = height = 0;
}


Renderer::~Renderer() {
}

rect Renderer::area() const {
	return rect(0, width, 0, height);
}
