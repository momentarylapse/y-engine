/*
 * Renderer.cpp
 *
 *  Created on: Jan 4, 2020
 *      Author: michi
 */

#include "Renderer.h"
#include "../lib/math/rect.h"
#include "../helper/PerformanceMonitor.h"


Renderer::Renderer() {
	width = height = 0;


	ch_render = PerformanceMonitor::create_channel("render");
	ch_end = PerformanceMonitor::create_channel("end", ch_render);
}


Renderer::~Renderer() {
}

rect Renderer::area() const {
	return rect(0, width, 0, height);
}
