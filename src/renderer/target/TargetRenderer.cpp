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
	for (auto c: children) {
		PerformanceMonitor::begin(channel);
		c->draw();
		PerformanceMonitor::end(channel);
	}
}
