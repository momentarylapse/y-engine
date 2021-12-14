/*
 * WorldRenderer.cpp
 *
 *  Created on: Jan 19, 2020
 *      Author: michi
 */

#include "WorldRenderer.h"
#include "../../helper/PerformanceMonitor.h"
#include "../../fx/Particle.h"
#include "../../gui/Picture.h"
#include "../../world/Camera.h"
#include "../../world/Light.h"
#include "../../world/Model.h"
#include "../../world/Terrain.h"
#include "../../world/World.h"
#include "../../lib/base/callable.h"


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

WorldRenderer::WorldRenderer(const string &name, Renderer *parent) : Renderer(name, parent) {
	ch_pre = PerformanceMonitor::create_channel("pre", channel);
	ch_post = PerformanceMonitor::create_channel("post", channel);
	ch_post_focus = PerformanceMonitor::create_channel("focus", ch_post);
	ch_bg = PerformanceMonitor::create_channel("bg", channel);
	ch_world = PerformanceMonitor::create_channel("world", channel);
	ch_fx = PerformanceMonitor::create_channel("fx", channel);
	ch_shadow = PerformanceMonitor::create_channel("shadow", channel);
	ch_prepare_lights = PerformanceMonitor::create_channel("lights", channel);
}


color WorldRenderer::background() const {
	return world.background;
}


void WorldRenderer::add_fx_injector(const RenderInjector::Callback *f) {
	fx_injectors.add({f});
}

void WorldRenderer::reset() {
	fx_injectors.clear();
}
