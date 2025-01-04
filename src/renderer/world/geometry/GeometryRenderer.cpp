/*
 * GeometryRenderer.cpp
 *
 *  Created on: Dec 15, 2022
 *      Author: michi
 */

#include "GeometryRenderer.h"
#include "../../base.h"
#include "../../../graphics-impl.h"
#include "../../../helper/PerformanceMonitor.h"

GeometryRendererCommon::GeometryRendererCommon(RenderPathType _type, SceneView &_scene_view) :
		Renderer("geo"),
		scene_view(_scene_view),
		fx_material(resource_manager)
{
	type = _type;
	flags = Flags::ALLOW_OPAQUE | Flags::ALLOW_TRANSPARENT;

	ch_pre = PerformanceMonitor::create_channel("pre", channel);
	ch_bg = PerformanceMonitor::create_channel("bg", channel);
	ch_fx = PerformanceMonitor::create_channel("fx", channel);
	ch_terrains = PerformanceMonitor::create_channel("ter", channel);
	ch_models = PerformanceMonitor::create_channel("mod", channel);
	ch_user = PerformanceMonitor::create_channel("usr", channel);
	ch_prepare_lights = PerformanceMonitor::create_channel("lights", channel);

	fx_material.pass0.cull_mode = 0;
	fx_material.pass0.mode = TransparencyMode::FUNCTIONS;
	fx_material.pass0.source = Alpha::SOURCE_ALPHA;
	fx_material.pass0.destination = Alpha::SOURCE_INV_ALPHA;
	fx_material.pass0.shader_path = "fx.shader";

	fx_vertex_buffers.add(new VertexBuffer("3f,4f,2f"));
}

Shader* GeometryRendererCommon::get_shader(Material* material, int pass_no, const string& vertex_shader_module, const string& geometry_shader_module) {
	if (!multi_pass_shader_cache[pass_no].contains(material))
		multi_pass_shader_cache[pass_no].set(material, {});
	auto& cache = multi_pass_shader_cache[pass_no][material];
	if (is_shadow_pass())
		cache._prepare_shader_multi_pass(type, *material_shadow, vertex_shader_module, geometry_shader_module, pass_no);
	else
		cache._prepare_shader_multi_pass(type, *material, vertex_shader_module, geometry_shader_module, pass_no);
	return cache.get_shader(type);
}

void GeometryRendererCommon::set(Flags _flags) {
	flags = _flags;
}

bool GeometryRendererCommon::is_shadow_pass() const {
	return (int)(flags & Flags::SHADOW_PASS);
}
