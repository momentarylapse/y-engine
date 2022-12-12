/*
 * ShadowRendererGL.cpp
 *
 *  Created on: Dec 11, 2022
 *      Author: michi
 */

#include "ShadowRendererGL.h"

#include <GLFW/glfw3.h>
#ifdef USING_OPENGL
#include "../WorldRendererGL.h"
#include "../../base.h"
#include "../../../lib/nix/nix.h"
#include "../../../helper/PerformanceMonitor.h"
#include "../../../world/Material.h"
#include "../../../Config.h"


ShadowRendererGL::ShadowRendererGL(Renderer *parent) : Renderer("shadow", parent) {
	int shadow_box_size = config.get_float("shadow.boxsize", 2000);
	int shadow_resolution = config.get_int("shadow.resolution", 1024);

	fb[0] = new nix::FrameBuffer({
		new nix::Texture(shadow_resolution, shadow_resolution, "rgba:i8"),
		new nix::DepthBuffer(shadow_resolution, shadow_resolution, "d24s8")});
	fb[1] = new nix::FrameBuffer({
		new nix::Texture(shadow_resolution, shadow_resolution, "rgba:i8"),
		new nix::DepthBuffer(shadow_resolution, shadow_resolution, "d24s8")});

	material = new Material;
	material->shader_path = "shadow.shader";
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
    auto w = static_cast<WorldRendererGL*>(parent);
	w->draw_terrains(false);
	w->draw_objects_instanced(false);
	w->draw_objects_opaque(false);
	w->draw_line_meshes(false);
	w->draw_point_meshes(false);
	w->draw_user_meshes(false, false, RenderPathType::FORWARD);

}

void ShadowRendererGL::prepare() {
	PerformanceMonitor::begin(channel);

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
