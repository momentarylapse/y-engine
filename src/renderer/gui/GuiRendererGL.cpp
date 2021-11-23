/*
 * GuiRendererGL.cpp
 *
 *  Created on: 23 Nov 2021
 *      Author: michi
 */

#include "GuiRendererGL.h"
#ifdef USING_OPENGL
#include "../base.h"
#include "../../lib/nix/nix.h"
#include "../../gui/gui.h"
#include "../../gui/Picture.h"
#include "../../helper/PerformanceMonitor.h"
#include "../../helper/ResourceManager.h"

GuiRendererGL::GuiRendererGL(Renderer *parent) : Renderer("gui", parent) {
	ch_gui = PerformanceMonitor::create_channel("gui", channel);
	shader = ResourceManager::load_shader("forward/2d.shader");
}

GuiRendererGL::~GuiRendererGL() {
}

void GuiRendererGL::draw() {
	if (child)
		child->draw();
	draw_gui(nullptr);
}

void GuiRendererGL::draw_gui(FrameBuffer *source) {
	PerformanceMonitor::begin(ch_gui);
	gui::update();

	nix::set_projection_ortho_relative();
	nix::set_cull(nix::CullMode::NONE);
	nix::set_alpha(nix::Alpha::SOURCE_ALPHA, nix::Alpha::SOURCE_INV_ALPHA);
	nix::set_z(false, false);

	for (auto *n: gui::sorted_nodes) {
		if (!n->eff_visible)
			continue;
		if (n->type == n->Type::PICTURE or n->type == n->Type::TEXT) {
			auto *p = (gui::Picture*)n;
			auto s = shader;
			if (p->shader)
				s = p->shader.get();
			nix::set_shader(s);
			s->set_float("blur", p->bg_blur);
			s->set_color("color", p->eff_col);
			nix::set_textures({p->texture.get()});// , source->color_attachments[0].get()});
			if (p->angle == 0) {
				nix::set_model_matrix(matrix::translation(vector(p->eff_area.x1, p->eff_area.y1, /*0.999f - p->eff_z/1000*/ 0.5f)) * matrix::scale(p->eff_area.width(), p->eff_area.height(), 0));
			} else {
				// TODO this should use the physical ratio
				float r = (float)width / (float)height;
				nix::set_model_matrix(matrix::translation(vector(p->eff_area.x1, p->eff_area.y1, /*0.999f - p->eff_z/1000*/ 0.5f)) * matrix::scale(1/r, 1, 0) * matrix::rotation_z(p->angle) * matrix::scale(p->eff_area.width() * r, p->eff_area.height(), 0));
			}
			gui::vertex_buffer->create_rect(rect::ID, p->source);
			nix::draw_triangles(gui::vertex_buffer);
		}
	}
	nix::set_z(true, true);
	nix::set_cull(nix::CullMode::DEFAULT);

	nix::disable_alpha();

	break_point();
	PerformanceMonitor::end(ch_gui);
}


#endif
