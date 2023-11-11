/*
 * WorldRenderer.cpp
 *
 *  Created on: Jan 19, 2020
 *      Author: michi
 */

#include "WorldRenderer.h"
#include "../../graphics-impl.h"
#include "../../helper/PerformanceMonitor.h"
#include "../../helper/ResourceManager.h"
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

WorldRenderer::WorldRenderer(const string &name, Camera *_cam) : Renderer(name) {
	ch_pre = PerformanceMonitor::create_channel("pre", channel);
	ch_post = PerformanceMonitor::create_channel("post", channel);
	ch_post_focus = PerformanceMonitor::create_channel("focus", ch_post);
	ch_bg = PerformanceMonitor::create_channel("bg", channel);
	ch_world = PerformanceMonitor::create_channel("world", channel);
	ch_fx = PerformanceMonitor::create_channel("fx", channel);
	ch_prepare_lights = PerformanceMonitor::create_channel("lights", channel);

	shadow_box_size = config.get_float("shadow.boxsize", 2000);
	shadow_resolution = config.get_int("shadow.resolution", 1024);
	cube_resolution = config.get_int("cubemap.resolution", 64);

	scene_view.cam = _cam;

	resource_manager->default_shader = "default.shader";
	if (config.get_str("renderer.shader-quality", "pbr") == "pbr") {
		resource_manager->load_shader_module("module-lighting-pbr.shader");
	} else {
		resource_manager->load_shader_module("module-lighting-simple.shader");
	}
	resource_manager->load_shader_module("module-vertex-default.shader");
	resource_manager->load_shader_module("module-vertex-animated.shader");
	resource_manager->load_shader_module("module-vertex-instanced.shader");
	resource_manager->load_shader_module("module-vertex-lines.shader");
	resource_manager->load_shader_module("module-vertex-points.shader");
	resource_manager->load_shader_module("module-vertex-fx.shader");
	resource_manager->load_shader_module("module-geometry-lines.shader");
	resource_manager->load_shader_module("module-geometry-points.shader");
}

WorldRenderer::~WorldRenderer() {
}


color WorldRenderer::background() const {
	return world.background;
}


void WorldRenderer::reset() {
}
