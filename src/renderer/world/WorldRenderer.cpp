/*
 * WorldRenderer.cpp
 *
 *  Created on: Jan 19, 2020
 *      Author: michi
 */

#include "WorldRenderer.h"
#include <lib/yrenderer/scene/pass/ShadowRenderer.h>
#include <lib/ygraphics/graphics-impl.h>


using namespace yrenderer;

/*struct GeoPush {
	alignas(16) mat4 model;
	alignas(16) color emission;
	alignas(16) vec3 eye_pos;
	alignas(16) float xxx[4];
};


mat4 mtr(const vec3 &t, const quaternion &a) {
	auto mt = mat4::translation(t);
	auto mr = mat4::rotation(a);
	return mt * mr;
}*/

WorldRenderer::WorldRenderer(Context* ctx, const string &name, SceneView& _scene_view) :
		Renderer(ctx, name),
		scene_view(_scene_view)
{
}

WorldRenderer::~WorldRenderer() = default;


void WorldRenderer::reset() {
}

void WorldRenderer::create_shadow_renderer(int resolution) {
	shadow_renderer = new ShadowRenderer(ctx, &scene_view, resolution);
	scene_view.shadow_maps.add(shadow_renderer->cascades[0].depth_buffer);
	scene_view.shadow_maps.add(shadow_renderer->cascades[1].depth_buffer);
	add_sub_task(shadow_renderer.get());
}
