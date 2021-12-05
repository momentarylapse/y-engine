/*
 * GuiRendererVulkan.cpp
 *
 *  Created on: 23 Nov 2021
 *      Author: michi
 */

#include "GuiRendererVulkan.h"
#ifdef USING_VULKAN
#include "../base.h"
#include "../../graphics-impl.h"
#include "../../gui/gui.h"
#include "../../gui/Picture.h"
#include "../../helper/PerformanceMonitor.h"
#include "../../helper/ResourceManager.h"
#include "../../lib/math/matrix.h"
#include "../../lib/math/rect.h"



struct UBOGUI {
	matrix m,v,p;
	color col;
	rect source;
	float blur, exposure, gamma;
};


void create_quad(VertexBuffer *vb, const rect &r, const rect &s = rect::ID);

GuiRendererVulkan::GuiRendererVulkan(Renderer *parent) : Renderer("gui", parent) {
	ch_gui = PerformanceMonitor::create_channel("gui", channel);


	shader = ResourceManager::load_shader("vulkan/2d.shader");
	pipeline = new vulkan::Pipeline(shader, parent->render_pass(), 0, "3f,3f,2f");
	pipeline->set_blend(Alpha::SOURCE_ALPHA, Alpha::SOURCE_INV_ALPHA);
	pipeline->set_z(false, false);
	pipeline->rebuild();


	vb = new VertexBuffer("3f,3f,2f");
	create_quad(vb, rect::ID);
}

GuiRendererVulkan::~GuiRendererVulkan() {
}

void GuiRendererVulkan::draw() {
	if (child)
		child->draw();
	prepare_gui(parent->frame_buffer());
	draw_gui(parent->command_buffer());
}

void GuiRendererVulkan::prepare_gui(FrameBuffer *source) {
	PerformanceMonitor::begin(ch_gui);
	gui::update();

	UBOGUI u;
	u.v = matrix::ID;
	u.p = matrix::scale(2.0f, 2.0f, 1) * matrix::translation(vector(-0.5f, -0.5f, 0)); // nix::set_projection_ortho_relative()
	u.gamma = 2.2f;
	u.exposure = 1.0f;

	int index = 0;

	for (auto *n: gui::sorted_nodes) {
		if (!n->eff_visible)
			continue;
		if (n->type == n->Type::PICTURE or n->type == n->Type::TEXT) {
			auto *p = (gui::Picture*)n;

			if (index >= ubo.num) {
				dset.add(pool->create_set("buffer,sampler"));
				ubo.add(new UniformBuffer(sizeof(UBOGUI)));
			}

			if (p->angle == 0) {
				u.m = matrix::translation(vector(p->eff_area.x1, p->eff_area.y1, /*0.999f - p->eff_z/1000*/ 0.5f)) * matrix::scale(p->eff_area.width(), p->eff_area.height(), 0);
			} else {
				// TODO this should use the physical ratio
				float r = (float)width / (float)height;
				u.m = matrix::translation(vector(p->eff_area.x1, p->eff_area.y1, /*0.999f - p->eff_z/1000*/ 0.5f)) * matrix::scale(1/r, 1, 0) * matrix::rotation_z(p->angle) * matrix::scale(p->eff_area.width() * r, p->eff_area.height(), 0);
			}
			u.blur = p->bg_blur;
			u.col = p->eff_col;
			u.source = p->source;
			ubo[index]->update(&u);

			dset[index]->set_buffer(0, ubo[index]);
			dset[index]->set_texture(1, p->texture.get());
//			dset[index]->set_texture(2, source->...);
			dset[index]->update();
			index ++;
		}
	}
	PerformanceMonitor::end(ch_gui);
}

void GuiRendererVulkan::draw_gui(CommandBuffer *cb) {
	PerformanceMonitor::begin(ch_gui);

	cb->bind_pipeline(pipeline);

	int index = 0;
	for (auto *n: gui::sorted_nodes) {
		if (!n->eff_visible)
			continue;
		if (n->type == n->Type::PICTURE or n->type == n->Type::TEXT) {

			cb->bind_descriptor_set(0, dset[index]);
			cb->draw(vb);
			index ++;
		}
	}

	//break_point();
	PerformanceMonitor::end(ch_gui);
}

#endif
