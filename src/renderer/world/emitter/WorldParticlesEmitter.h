//
// Created by Michael Ankele on 2025-05-07.
//

#pragma once

#include "../../scene/MeshEmitter.h"
#include <graphics-fwd.h>
#include <world/Material.h>


class WorldParticlesEmitter : public MeshEmitter {
public:
	WorldParticlesEmitter();
	void emit_transparent(const RenderParams& params, RenderViewData& rvd) override;

//	shared<Shader> shader_fx;
//	shared<Shader> shader_fx_points;
	owned<VertexBuffer> vb_fx;
	owned<VertexBuffer> vb_fx_points;

	Material fx_material;

	owned_array<VertexBuffer> fx_vertex_buffers;
};

