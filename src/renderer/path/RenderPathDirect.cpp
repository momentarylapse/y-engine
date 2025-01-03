//
// Created by michi on 1/3/25.
//

#include "RenderPathDirect.h"
#include "../world/WorldRenderer.h"
#include "../world/pass/ShadowRenderer.h"


RenderPathDirect::RenderPathDirect(Camera* cam) : RenderPath(RenderPathType::Direct, cam) {
	world_renderer = create_world_renderer(cam, scene_view, main_rvd, RenderPathType::Forward);
	create_shadow_renderer();
	create_geometry_renderer();
	world_renderer->geo_renderer = geo_renderer.get();
}

void RenderPathDirect::prepare(const RenderParams &params) {
	prepare_basics();
	prepare_lights(scene_view.cam, main_rvd);
	if (scene_view.shadow_index >= 0) {
		shadow_renderer->set_scene(scene_view);
		shadow_renderer->render(params);
	}
	world_renderer->prepare(params);
}

void RenderPathDirect::draw(const RenderParams &params) {
	world_renderer->draw(params);
}

