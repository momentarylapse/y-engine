/*
 * TargetRenderer.cpp
 *
 *  Created on: Nov 23, 2021
 *      Author: michi
 */

#include "TargetRenderer.h"
#include "../../helper/PerformanceMonitor.h"


TargetRenderer::TargetRenderer(const string &name) : Renderer(name, nullptr) {
	ch_end = PerformanceMonitor::create_channel("end", channel);
}


TargetRenderer::~TargetRenderer() {
}

void TargetRenderer::draw() {
	if (child) {
		PerformanceMonitor::begin(channel);
		child->draw();
		PerformanceMonitor::end(channel);
	}
}
