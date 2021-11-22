/*
 * Renderer.cpp
 *
 *  Created on: Jan 4, 2020
 *      Author: michi
 */

#include "Renderer.h"
#include "RenderPath.h"
#include "../lib/math/rect.h"
#include "../helper/PerformanceMonitor.h"


Renderer::Renderer() {
	width = height = 0;
	render_path = nullptr;


	ch_render = PerformanceMonitor::create_channel("render");
	ch_end = PerformanceMonitor::create_channel("end", ch_render);
}


Renderer::~Renderer() {
}

rect Renderer::area() const {
	return rect(0, width, 0, height);
}

void Renderer::set_render_path(RenderPath *rp) {
	render_path = rp;
}

void Renderer::draw_frame() {
	render_path->draw();
}
