/*
 * RegionRendererGL.cpp
 *
 *  Created on: 11 Oct 2023
 *      Author: michi
 */

#include "../regions/RegionRendererGL.h"

#ifdef USING_OPENGL
#include "../base.h"
#include "../../lib/nix/nix.h"

RegionRendererGL::RegionRendererGL(Renderer *parent) : Renderer("region", parent) {
}

void RegionRendererGL::draw() {
	for (int i=0; i<children.num; i++)
		regions[i].renderer = children[i];

	const rect area = frame_buffer()->area();

	for (auto& r: regions) {
		if (r.renderer) {
			nix::set_viewport(rect(area.x2 * r.dest.x1, area.x2 * r.dest.x2, area.y2 * r.dest.y1, area.y2 * r.dest.y2));
			r.renderer->draw();
		}
	}
	nix::set_viewport(area);
}

Renderer* RegionRendererGL::add_region(const rect &dest) {
	regions.add({dest, nullptr});
	return this;
}

#endif


