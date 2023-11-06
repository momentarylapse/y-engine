/*
 * RegionRendererVulkan.cpp
 *
 *  Created on: 11 Oct 2023
 *      Author: michi
 */

#include "RegionRendererVulkan.h"

#ifdef USING_VULKAN
#include "../base.h"
#include <graphics-impl.h>

RegionRendererVulkan::RegionRendererVulkan(Renderer *parent) : Renderer("region", parent) {
}

void RegionRendererVulkan::prepare(const RenderParams& params) {
	for (int i=0; i<children.num; i++)
		regions[i].renderer = children[i];

	for (auto& r: regions) {
		if (r.renderer) {
			auto sub_params = params;
			sub_params.desired_aspect_ratio *= r.dest.width() / r.dest.height();
			r.renderer->prepare(sub_params);
		}
	}
}

void RegionRendererVulkan::draw(const RenderParams& params) {
	auto cb = params.command_buffer;
	auto fb = params.frame_buffer;
	const rect area = rect(0, fb->width, 0, fb->height);

	for (auto& r: regions) {
		if (r.renderer) {
			auto sub_params = params;
			sub_params.desired_aspect_ratio *= r.dest.width() / r.dest.height();
			auto rr = rect(area.x2 * r.dest.x1, area.x2 * r.dest.x2, area.y2 * r.dest.y1, area.y2 * r.dest.y2);
			cb->set_viewport(rr);
			r.renderer->draw(sub_params);
		}
	}
	cb->set_viewport(area);
}

Renderer* RegionRendererVulkan::add_region(const rect &dest) {
	regions.add({dest, nullptr});
	return this;
}

#endif

