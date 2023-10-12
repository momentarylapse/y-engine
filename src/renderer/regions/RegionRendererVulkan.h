/*
 * RegionRendererVulkan.h
 *
 *  Created on: 11 Oct 2023
 *      Author: michi
 */

#ifndef SRC_RENDERER_REGIONS_REGIONRENDERERVULKAN_H_
#define SRC_RENDERER_REGIONS_REGIONRENDERERVULKAN_H_

#include "../Renderer.h"
#ifdef USING_VULKAN

#include <lib/math/rect.h>

class RegionRendererVulkan : public Renderer {
public:
	RegionRendererVulkan(Renderer *parent);

	void prepare(const RenderParams& params);
	void draw(const RenderParams& params) override;

	Renderer *add_region(const rect &dest);

	struct Region {
		rect dest;
		Renderer *renderer;
	};

	Array<Region> regions;
};

#endif

#endif /* SRC_RENDERER_REGIONS_REGIONRENDERERVULKAN_H_ */
