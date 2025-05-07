//
// Created by michi on 1/3/25.
//

#include "RenderPathDirect.h"
#include "../world/WorldRenderer.h"
#include "../world/pass/ShadowRenderer.h"
#include "../world/geometry/SceneView.h"
#include "../x/WorldModelsEmitter.h"
#include "../x/WorldTerrainsEmitter.h"
#include "../x/WorldSkyboxEmitter.h"
#include "../x/WorldParticlesEmitter.h"
#include <helper/ResourceManager.h>
#include <world/World.h>
#include "../../world/Camera.h"


RenderPathDirect::RenderPathDirect(Camera* cam) : RenderPath(RenderPathType::Direct, cam) {
	resource_manager->load_shader_module("forward/module-surface.shader");

	scene_renderer = new SceneRenderer(scene_view);
	scene_renderer->add_emitter(new WorldSkyboxEmitter);
	scene_renderer->add_emitter(new WorldModelsEmitter);
	scene_renderer->add_emitter(new WorldTerrainsEmitter);
	scene_renderer->add_emitter(new WorldParticlesEmitter);

	create_shadow_renderer();
}

void RenderPathDirect::prepare(const RenderParams &params) {
	prepare_basics();
	scene_view.choose_lights();
	scene_view.choose_shadows();
	scene_renderer->background_color = world.background;
	scene_renderer->set_view_from_camera(params, cam);
	scene_renderer->prepare(params);

	if (shadow_renderer)
		for (int i: scene_view.shadow_indices) {
			shadow_renderer->set_projection(scene_view.lights[i]->shadow_projection);
			shadow_renderer->render(params);
		}
}

void RenderPathDirect::draw(const RenderParams &params) {
	scene_renderer->draw(params);
}

