/*
 * WorldRenderer.cpp
 *
 *  Created on: Jan 19, 2020
 *      Author: michi
 */

#include "WorldRenderer.h"
#include "../../graphics-impl.h"
#include "../../helper/PerformanceMonitor.h"
#include "../../fx/Particle.h"
#include "../../gui/Picture.h"
#include "../../world/Camera.h"
#include "../../world/Light.h"
#include "../../world/Model.h"
#include "../../world/Terrain.h"
#include "../../world/World.h"
#include "../../lib/base/callable.h"
#include "../../Config.h"


struct GeoPush {
	alignas(16) mat4 model;
	alignas(16) color emission;
	alignas(16) vec3 eye_pos;
	alignas(16) float xxx[4];
};


mat4 mtr(const vec3 &t, const quaternion &a) {
	auto mt = mat4::translation(t);
	auto mr = mat4::rotation(a);
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

	using_view_space = true;

	shadow_box_size = config.get_float("shadow.boxsize", 2000);
	shadow_resolution = config.get_int("shadow.resolution", 1024);
	shadow_index = -1;
}

WorldRenderer::~WorldRenderer() {
}


color WorldRenderer::background() const {
	return world.background;
}


void WorldRenderer::add_fx_injector(const RenderInjector::Callback *f, bool transparent) {
	fx_injectors.add({f, transparent});
}

void WorldRenderer::reset() {
	fx_injectors.clear();
}
