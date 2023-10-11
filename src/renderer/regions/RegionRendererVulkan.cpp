/*
 * RegionRendererVulkan.cpp
 *
 *  Created on: 11 Oct 2023
 *      Author: michi
 */

#include "RegionRendererVulkan.h"

#ifdef USING_VULKAN
#include "../base.h"

RegionRendererVulkan::RegionRendererVulkan(Renderer *parent) : Renderer("region", parent) {
}

void RegionRendererVulkan::draw() {
	for (int i=0; i<children.num; i++)
		regions[i].renderer = children[i];

	for (auto& r: regions) {
		if (r.renderer) {
			r.renderer->draw();
		}
	}
}

Renderer* RegionRendererVulkan::add_region(const rect &dest) {
	regions.add({dest, nullptr});
	return this;
}

#endif

