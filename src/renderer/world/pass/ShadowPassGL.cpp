/*
 * ShadowPassGL.cpp
 *
 *  Created on: Dec 11, 2022
 *      Author: michi
 */

#include "ShadowPassGL.h"

#include <GLFW/glfw3.h>
#ifdef USING_OPENGL
#include "../WorldRendererGL.h"
#include "../../base.h"
#include "../../../lib/nix/nix.h"


ShadowPassGL::ShadowPassGL(Renderer *parent) : Renderer("shadow", parent) {
}

void ShadowPassGL::set(const mat4 &_shadow_proj, float _scale) {
    shadow_proj = _shadow_proj;
    scale = _scale;
}

void ShadowPassGL::prepare() {
}

void ShadowPassGL::draw() {
	auto m = mat4::scale(scale, scale, 1);
	//m = m * jitter(sfb->width*8, sfb->height*8, 1);
	nix::set_projection_matrix(m * shadow_proj);
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

	break_point();
}


#endif
