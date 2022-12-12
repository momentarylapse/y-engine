/*
 * ShadowRendererVulkan.cpp
 *
 *  Created on: Dec 11, 2022
 *      Author: michi
 */

#include "ShadowRendererVulkan.h"

#include <GLFW/glfw3.h>
#ifdef USING_VULKAN
#include "../WorldRendererVulkan.h"
#include "../../base.h"
#include "../../../graphics-impl.h"
#include "../../../world/Light.h"
#include "../../../world/Material.h"
#include "../../../lib/nix/nix.h"
#include "../../../Config.h"


ShadowRendererVulkan::ShadowRendererVulkan(Renderer *parent) : Renderer("shadow", parent) {
	int shadow_box_size = config.get_float("shadow.boxsize", 2000);
	int shadow_resolution = config.get_int("shadow.resolution", 1024);

	auto tex1 = new vulkan::Texture(shadow_resolution, shadow_resolution, "rgba:i8");
	auto tex2 = new vulkan::Texture(shadow_resolution, shadow_resolution, "rgba:i8");
	auto depth1 = new vulkan::DepthBuffer(shadow_resolution, shadow_resolution, "d:f32", true);
	auto depth2 = new vulkan::DepthBuffer(shadow_resolution, shadow_resolution, "d:f32", true);
	_render_pass = new vulkan::RenderPass({tex1, depth1}, "clear");
	fb[0] = new vulkan::FrameBuffer(render_pass(), {tex1, depth1});
	fb[1] = new vulkan::FrameBuffer(render_pass(), {tex2, depth2});


	rvd[0].ubo_light = new UniformBuffer(3 * sizeof(UBOLight)); // just to fill the dset
	rvd[1].ubo_light = new UniformBuffer(3 * sizeof(UBOLight));

	material = new Material;
	material->shader_path = "shadow.shader";
}

void ShadowRendererVulkan::render(vulkan::CommandBuffer *cb, const mat4 &m) {
	proj = m;
	render_shadow_map(cb, fb[0].get(), 4, rvd[0]);
	render_shadow_map(cb, fb[1].get(), 1, rvd[1]);
}

void ShadowRendererVulkan::prepare() {
}

void ShadowRendererVulkan::render_shadow_map(CommandBuffer *cb, FrameBuffer *sfb, float scale, RenderViewDataVK &rvd) {

	cb->begin_render_pass(render_pass(), sfb);
	cb->set_viewport(rect(0, sfb->width, 0, sfb->height));

	//shadow_pass->set(shadow_proj, scale, &rvd);
	//shadow_pass->draw();

	auto m = mat4::scale(scale, -scale, 1);
	//m = m * jitter(sfb->width*8, sfb->height*8, 1);

	UBO ubo;
	ubo.p = m * proj;
	ubo.v = mat4::ID;
	ubo.num_lights = 0;
	ubo.shadow_index = -1;

	auto w = static_cast<WorldRendererVulkan*>(parent);
	w->draw_terrains(cb, _render_pass, ubo, false, rvd);
	w->draw_objects_opaque(cb, _render_pass, ubo, false, rvd);
	w->draw_objects_instanced(cb, _render_pass, ubo, false, rvd);


	cb->end_render_pass();
}

#endif