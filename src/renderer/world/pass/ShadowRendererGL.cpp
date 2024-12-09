/*
 * ShadowRendererGL.cpp
 *
 *  Created on: Dec 11, 2022
 *      Author: michi
 */

#include "ShadowRendererGL.h"

#ifdef USING_OPENGL
#include "../geometry/GeometryRendererGL.h"
#include "../../target/TextureRendererGL.h"
#include "../WorldRendererGL.h"
#include "../../base.h"
#include "../../../graphics-impl.h"
#include "../../../helper/PerformanceMonitor.h"
#include <world/Material.h>
#include <world/Camera.h>
#include "../../../Config.h"

ShadowRenderer::Cascade::Cascade() = default;
ShadowRenderer::Cascade::~Cascade() = default;

ShadowRenderer::ShadowRenderer() :
		RenderTask("shdw")
{
	//int shadow_box_size = config.get_float("shadow.boxsize", 2000);
	int shadow_resolution = config.get_int("shadow.resolution", 1024);

	material = new Material(resource_manager);
	material->pass0.shader_path = "shadow.shader";

	geo_renderer = new GeometryRendererGL(RenderPathType::FORWARD, scene_view);
	geo_renderer->flags = GeometryRenderer::Flags::SHADOW_PASS;
	geo_renderer->material_shadow = material.get();

	for (int i=0; i<NUM_CASCADES; i++) {
		auto& c = cascades[i];
		shared tex = new Texture(shadow_resolution, shadow_resolution, "rgba:i8");
		c.depth_buffer = new DepthBuffer(shadow_resolution, shadow_resolution, "d24s8");
		c.texture_renderer = new TextureRenderer({tex, c.depth_buffer});
		c.fb = c.texture_renderer->frame_buffer;
		c.scale = (i == 0) ? 4.0f : 1.0f;
		c.texture_renderer->add_child(geo_renderer.get());
	}
}

void ShadowRenderer::render_cascade(Cascade& c) {
	const auto params = RenderParams::into_texture(c.fb.get(), 1.0f);
	geo_renderer->prepare(params);

	nix::bind_frame_buffer(c.fb.get());

	auto m = mat4::scale(c.scale, c.scale, 1);
	//m = m * jitter(sfb->width*8, sfb->height*8, 1);
	c.rvd.set_projection_matrix(m * proj);
	c.rvd.set_view_matrix(mat4::ID);

	nix::clear_z();

	nix::set_z(true, true);

	//c.texture_renderer->render(params);

    // all opaque meshes
	geo_renderer->draw_opaque(params, c.rvd);

}

void ShadowRenderer::set_scene(SceneView &parent_scene_view) {
	scene_view.cam = parent_scene_view.cam;
	proj = parent_scene_view.shadow_proj;
}

void ShadowRenderer::render(const RenderParams& params) {
	PerformanceMonitor::begin(ch_prepare);
	gpu_timestamp_begin(ch_prepare);

	render_cascade(cascades[0]);
	render_cascade(cascades[1]);

	gpu_timestamp_end(ch_prepare);
	PerformanceMonitor::end(ch_prepare);
}


#endif
