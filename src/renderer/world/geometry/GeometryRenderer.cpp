/*
 * GeometryRenderer.cpp
 *
 *  Created on: Dec 15, 2022
 *      Author: michi
 */

#include "GeometryRenderer.h"
#include "../../base.h"
#include "../../../graphics-impl.h"
#include "../../../helper/PerformanceMonitor.h"

bool GeometryRenderer::using_view_space = true;

GeometryRenderer::GeometryRenderer(RenderPathType _type, Renderer *parent) : Renderer("geo", parent) {
	type = _type;

	ch_pre = PerformanceMonitor::create_channel("pre", channel);
	ch_bg = PerformanceMonitor::create_channel("bg", channel);
	ch_world = PerformanceMonitor::create_channel("world", channel);
	ch_fx = PerformanceMonitor::create_channel("fx", channel);
	ch_prepare_lights = PerformanceMonitor::create_channel("lights", channel);

	using_view_space = true;
}
