/*
 * Layer.cpp
 *
 *  Created on: Aug 10, 2020
 *      Author: michi
 */

#include "Node.h"
//#include <algorithm>

namespace gui {

Node::Node(const rect &r) {
	type = Type::NODE;
	area = r;
	dz = 1;
	col = White;
	visible = true;
	margin = rect::EMPTY;
	align = Align::_TOP_LEFT;

	eff_col = White;
	eff_area = r;
	eff_z = 0;

	parent = nullptr;
}

Node::~Node() {
}

void Node::__init__(const rect &r) {
	new(this) Node(r);
}

void Node::__delete__() {
	this->~Node();
}

void Node::add(Node *n) {
	children.add(n);
	n->parent = this;
	all_nodes.add(n);
}

rect rect_move_x(const rect &r, float dx) {
	return rect(r.x1 + dx, r.x2 + dx, r.y1, r.y2);
}

rect rect_move_y(const rect &r, float dy) {
	return rect(r.x1, r.x2, r.y1 + dy, r.y2 + dy);
}

rect rect_sub_margin(const rect &r, const rect &m) {
	return rect(r.x1 + m.x1, r.x2 - m.x2, r.y1 + m.y1, r.y2 - m.y2);
}

void Node::update_geometry(const rect &target) {
	if (parent) {
		eff_z = parent->eff_z + dz;
		eff_col = col;//parent->eff_col * col;
	} else {
		// toplevel
		eff_col = col;
		eff_z = 0;
	}

	if (parent) {
		eff_area = target;
		if (align & Align::FILL_X) {
			eff_area.x1 = target.x1 + margin.x1;
			eff_area.x2 = target.x2 - margin.x2;
		} else if (align & Align::LEFT) {
			eff_area.x1 = target.x1 + area.x1 + margin.x1;
			eff_area.x2 = target.x1 + area.x2 + margin.x1;
		} else if (align & Align::RIGHT) {
			eff_area.x1 = target.x2 + area.x1 - margin.x2;
			eff_area.x2 = target.x2 + area.x2 - margin.x2;
		}

		if (align & Align::FILL_Y) {
			eff_area.y1 = target.y1 + margin.y1;
			eff_area.y2 = target.y2 - margin.y2;
		} else if (align & Align::TOP) {
			eff_area.y1 = target.y1 + area.y1 + margin.y1;
			eff_area.y2 = target.y1 + area.y2 + margin.y1;
		} else if (align & Align::BOTTOM) {
			eff_area.y1 = target.y2 + area.y1 - margin.y2;
			eff_area.y2 = target.y2 + area.y2 - margin.y2;
		}

		//eff_area = rect_sub_margin(eff_area, margin);

	} else {
		// toplevel
		eff_area = target;
	}

	auto sub_area = eff_area;
	for (auto n: children) {
		n->update_geometry(sub_area);
		if (type == Type::VBOX)
			sub_area.y1 = n->eff_area.y2 + n->margin.y2;
		if (type == Type::HBOX)
			sub_area.x1 = n->eff_area.x2 + n->margin.x2;
	}
}


HBox::HBox() : Node(rect::ID) {
	type = Type::HBOX;
	align = Align::_FILL_XY;
}

void HBox::__init__() {
	new(this) HBox();
}



VBox::VBox() : Node(rect::ID) {
	type = Type::VBOX;
	align = Align::_FILL_XY;
}

void VBox::__init__() {
	new(this) VBox();
}


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

void handle_input(float mx, float my, std::function<bool(Node *n)> f) {
	foreachb(Node *n, sorted_nodes) {
		if (n->eff_area.inside(mx, my)) {
			if (f(n))
				return;
		}
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
