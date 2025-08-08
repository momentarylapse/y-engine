/*
 * WorldRendererForward.cpp
 *
 *  Created on: Jun 2, 2021
 *      Author: michi
 */

#include "WorldRendererForward.h"
#include <lib/yrenderer/scene/pass/ShadowRenderer.h>
#include <lib/yrenderer/Context.h>
#include <lib/yrenderer/helper/CubeMapSource.h>
#include <lib/image/image.h>
#include <lib/profiler/Profiler.h>
#include <lib/yrenderer/ShaderManager.h>


WorldRendererForward::WorldRendererForward(yrenderer::Context* ctx, yrenderer::SceneView& scene_view) : WorldRenderer(ctx, "world", scene_view) {
	shader_manager->load_shader_module("forward/module-surface.shader");

	scene_renderer = new yrenderer::SceneRenderer(ctx, yrenderer::RenderPathType::Forward, scene_view);
}


void WorldRendererForward::add_background_emitter(shared<yrenderer::MeshEmitter> emitter) {
	scene_renderer->add_emitter(emitter);
}

void WorldRendererForward::add_opaque_emitter(shared<yrenderer::MeshEmitter> emitter) {
	scene_renderer->add_emitter(emitter);
}

void WorldRendererForward::add_transparent_emitter(shared<yrenderer::MeshEmitter> emitter) {
	scene_renderer->add_emitter(emitter);
}




void WorldRendererForward::prepare(const yrenderer::RenderParams& params) {
	profiler::begin(ch_prepare);
	
	scene_renderer->set_view(params, view);
	scene_renderer->prepare(params);

	profiler::end(ch_prepare);
}

void WorldRendererForward::draw(const yrenderer::RenderParams& params) {
	profiler::begin(channel);
	ctx->gpu_timestamp_begin(params, channel);

	scene_renderer->draw(params);

	ctx->gpu_timestamp_end(params, channel);
	profiler::end(channel);
}

