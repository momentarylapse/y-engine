/*
 * ShadowRendererGL.cpp
 *
 *  Created on: Dec 11, 2022
 *      Author: michi
 */

#include "ShadowRendererGL.h"

#include <GLFW/glfw3.h>
#ifdef USING_OPENGL
#include "../geometry/GeometryRendererGL.h"
#include "../WorldRendererGL.h"
#include "../../base.h"
#include "../../../lib/nix/nix.h"
#include "../../../helper/PerformanceMonitor.h"
#include "../../../world/Material.h"
#include "../../../world/Camera.h"
#include "../../../Config.h"


ShadowRendererGL::ShadowRendererGL(Renderer *parent) : Renderer("shadow", parent) {
	//int shadow_box_size = config.get_float("shadow.boxsize", 2000);
	int shadow_resolution = config.get_int("shadow.resolution", 1024);

	fb[0] = new nix::FrameBuffer({
		new nix::Texture(shadow_resolution, shadow_resolution, "rgba:i8"),
		new nix::DepthBuffer(shadow_resolution, shadow_resolution, "d24s8")});
	fb[1] = new nix::FrameBuffer({
		new nix::Texture(shadow_resolution, shadow_resolution, "rgba:i8"),
		new nix::DepthBuffer(shadow_resolution, shadow_resolution, "d24s8")});

	material = new Material;
	material->shader_path = "shadow.shader";

	geo_renderer = new GeometryRendererGL(RenderPathType::FORWARD, this);
	geo_renderer->flags = GeometryRenderer::Flags::SHADOW_PASS;
	geo_renderer->material_shadow = material;
	geo_renderer->num_lights = 0;
}

void ShadowRendererGL::render_shadow_map(FrameBuffer *sfb, float scale) {
	nix::bind_frame_buffer(sfb);

	auto m = mat4::scale(scale, scale, 1);
	//m = m * jitter(sfb->width*8, sfb->height*8, 1);
	nix::set_projection_matrix(m * proj);
	nix::set_view_matrix(mat4::ID);
	nix::set_model_matrix(mat4::ID);

	nix::clear_z();

	nix::set_z(true, true);

    // all opaque meshes
	geo_renderer->draw_opaque();

}

void ShadowRendererGL::prepare() {
	PerformanceMonitor::begin(channel);


	geo_renderer->cam = cam_main;
	geo_renderer->prepare();

	render_shadow_map(fb[1].get(), 1);
	render_shadow_map(fb[0].get(), 4);


	break_point();
	PerformanceMonitor::end(channel);
}

void ShadowRendererGL::render(const mat4 &m) {
	proj = m;
	prepare();
}


#endif
