/*
 * RenderPath.cpp
 *
 *  Created on: Jan 19, 2020
 *      Author: michi
 */

#include "RenderPath.h"
#include "Renderer.h"
#include "../helper/PerformanceMonitor.h"
#include "../fx/Particle.h"
#include "../gui/Picture.h"
#include "../world/Camera.h"
#include "../world/Light.h"
#include "../world/Model.h"
#include "../world/Terrain.h"
#include "../world/World.h"
#include "../lib/base/callable.h"


struct GeoPush {
	alignas(16) matrix model;
	alignas(16) color emission;
	alignas(16) vector eye_pos;
	alignas(16) float xxx[4];
};


matrix mtr(const vector &t, const quaternion &a) {
	auto mt = matrix::translation(t);
	auto mr = matrix::rotation_q(a);
	return mt * mr;
}

RenderPath::RenderPath() {
	ch_render = PerformanceMonitor::create_channel("render");
	ch_pre = PerformanceMonitor::create_channel("pre", ch_render);
	ch_gui = PerformanceMonitor::create_channel("gui", ch_render);
	ch_out = PerformanceMonitor::create_channel("out", ch_render);
	ch_post = PerformanceMonitor::create_channel("post", ch_render);
	ch_post_blur = PerformanceMonitor::create_channel("blur", ch_post);
	ch_post_focus = PerformanceMonitor::create_channel("focus", ch_post);
	ch_end = PerformanceMonitor::create_channel("end", ch_render);
	ch_bg = PerformanceMonitor::create_channel("bg", ch_render);
	ch_world = PerformanceMonitor::create_channel("world", ch_render);
	ch_fx = PerformanceMonitor::create_channel("fx", ch_render);
	ch_shadow = PerformanceMonitor::create_channel("shadow", ch_render);
	ch_prepare_lights = PerformanceMonitor::create_channel("lights", ch_render);
}


string callable_name(const void *c);

void RenderPath::add_post_processor(const PostProcessor::Callback *f) {
	post_processors.add({f, PerformanceMonitor::create_channel(callable_name(f), ch_post)});
}

void RenderPath::add_fx_injector(const RenderInjector::Callback *f) {
	fx_injectors.add({f});
}

void RenderPath::reset() {
	post_processors.clear();
	fx_injectors.clear();
}
