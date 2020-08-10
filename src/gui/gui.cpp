/*
 * gui.cpp
 *
 *  Created on: Aug 11, 2020
 *      Author: michi
 */

#include "gui.h"
#include "Node.h"

namespace gui {

nix::Shader *shader = nullptr;
nix::VertexBuffer *vertex_buffer = nullptr;
Array<Node*> all_nodes;
Array<Node*> sorted_nodes;
Node *toplevel = nullptr;


void init(nix::Shader *s) {
	vertex_buffer = new nix::VertexBuffer("3f,3f,2f");
	vertex_buffer->create_rect(rect::ID);

	shader = s;
	toplevel = new Node(rect::ID);
}

/*vulkan::RenderPass *rp) {
	Picture::render_pass = rp;
	Picture::shader = vulkan::Shader::load("2d.shader");
	Picture::pipeline = new vulkan::Pipeline(Picture::shader, rp, 0, 1);
	Picture::pipeline->set_blend(VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
	Picture::pipeline->set_z(false, false);
	Picture::pipeline->rebuild();

	Picture::vertex_buffer = new vulkan::VertexBuffer();
	Array<vulkan::Vertex1> vertices;
	vertices.add({vector(0,0,0), vector::EZ, 0,0});
	vertices.add({vector(0,1,0), vector::EZ, 0,1});
	vertices.add({vector(1,0,0), vector::EZ, 1,0});
	vertices.add({vector(1,1,0), vector::EZ, 1,1});
	Picture::vertex_buffer->build1i(vertices, {0,2,1, 1,2,3});
}*/

void reset() {
	for (auto *n: all_nodes) {
		n->children.clear();
		n->parent = nullptr;
		delete n;
	}
	all_nodes.clear();

	toplevel = new Node(rect::ID);
}

void update() {
	toplevel->update_geometry(rect::ID);

	sorted_nodes = all_nodes;
	//std::sort(sorted_nodes.begin(), sorted_nodes.end(), [](Node *a, Node *b) { return a->eff_z < b->eff_z; });
	for (int i=0; i<sorted_nodes.num; i++)
		for (int j=i+1; j<sorted_nodes.num; j++)
			if (sorted_nodes[i]->eff_z < sorted_nodes[j]->eff_z)
				sorted_nodes.swap(i, j);

	for (auto *p: all_nodes) {
		//p->rebuild();
	}
}

void handle_input(const vector &m, std::function<bool(Node *n)> f) {
	foreachb(Node *n, sorted_nodes) {
		if (n->eff_area.inside(m.x, m.y)) {
			if (f(n))
				return;
		}
	}
}

void handle_mouse_move(const vector &m_prev, const vector &m) {
	for (auto n: all_nodes) {
		if (n->eff_area.inside(m.x, m.y) and !n->eff_area.inside(m_prev.x, m_prev.y))
			n->on_enter();
		if (!n->eff_area.inside(m.x, m.y) and n->eff_area.inside(m_prev.x, m_prev.y))
			n->on_leave();
	}
}

void iterate(float dt) {
	for (auto n: all_nodes) {
		n->on_iterate(dt);
	}
}

#if 0
void render(vulkan::CommandBuffer *cb, const rect &viewport) {
	cb->set_pipeline(Picture::pipeline);
	cb->set_viewport(viewport);

	for (auto *p: pictures) {
		if (p->user_shader)
			continue;

		UBOMatrices u;
		u.proj = matrix::translation(vector(-1,-1,0)) * matrix::scale(2,2,1);
		u.view = matrix::ID;
		u.model = matrix::translation(p->pos) * matrix::scale(p->width, p->height, 1);
		p->ubo->update(&u);

		cb->bind_descriptor_set(0, p->dset);
		cb->draw(Picture::vertex_buffer);
	}

	for (auto *p: pictures) {
		if (!p->user_shader)
			continue;
		cb->set_pipeline(p->user_pipeline);
		cb->set_viewport(viewport);

		UBOMatrices u;
		u.proj = matrix::translation(vector(-1,-1,0)) * matrix::scale(2,2,1);
		u.view = matrix::ID;
		u.model = matrix::translation(p->pos) * matrix::scale(p->width, p->height, 1);
		p->ubo->update(&u);

		cb->bind_descriptor_set(0, p->dset);
		cb->draw(Picture::vertex_buffer);
	}
}
#endif



}
