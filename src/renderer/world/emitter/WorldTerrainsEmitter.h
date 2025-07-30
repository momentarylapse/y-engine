//
// Created by Michael Ankele on 2025-05-06.
//

#pragma once

#include <lib/yrenderer/scene/MeshEmitter.h>

class WorldTerrainsEmitter : public yrenderer::MeshEmitter {
public:
	WorldTerrainsEmitter();
	void emit(const yrenderer::RenderParams& params, yrenderer::RenderViewData& rvd, bool shadow_pass) override;
};


