/*
 * ShadowPassVulkan.cpp
 *
 *  Created on: Dec 11, 2022
 *      Author: michi
 */

#include "ShadowPassVulkan.h"

#include <GLFW/glfw3.h>
#ifdef USING_VULKAN
#include "../WorldRendererVulkan.h"
#include "../../base.h"
#include "../../../graphics-impl.h"
#include "../../../lib/nix/nix.h"


ShadowPassVulkan::ShadowPassVulkan(Renderer *parent, vulkan::Texture *tex, vulkan::DepthBuffer *depth) : Renderer("shadow", parent) {
	_render_pass = new vulkan::RenderPass({tex, depth}, "clear");
}

void ShadowPassVulkan::set(const mat4 &_shadow_proj, float _scale, RenderViewDataVK *_rvd) {
    shadow_proj = _shadow_proj;
    scale = _scale;
	rvd = _rvd;
}

void ShadowPassVulkan::prepare() {
}

void ShadowPassVulkan::draw() {
	auto cb = command_buffer();


	//cb->begin_render_pass(render_pass_shadow, sfb);
	//cb->set_viewport(rect(0, sfb->width, 0, sfb->height));

	auto m = mat4::scale(scale, -scale, 1);
	//m = m * jitter(sfb->width*8, sfb->height*8, 1);

	UBO ubo;
	ubo.p = m * shadow_proj;
	ubo.v = mat4::ID;
	ubo.num_lights = 0;
	ubo.shadow_index = -1;

	auto w = static_cast<WorldRendererVulkan*>(parent);
	w->draw_terrains(cb, _render_pass, ubo, false, *rvd);
	w->draw_objects_opaque(cb, _render_pass, ubo, false, *rvd);
	w->draw_objects_instanced(cb, _render_pass, ubo, false, *rvd);
}


#endif
