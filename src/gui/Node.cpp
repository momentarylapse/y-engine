/*
 * Layer.cpp
 *
 *  Created on: Aug 10, 2020
 *      Author: michi
 */

#include "Node.h"

namespace gui {

Node::Node(const rect &r) {
	type = Type::NODE;
	area = r;
	dz = 1;
	col = White;
	visible = true;
	margin = rect::EMPTY;
	content_align_x = content_align_y = Align::NONE;

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


HBox::HBox(const rect &r) : Node(r) {
	type = Type::HBOX;
	content_align_x = Align::FILL;
	content_align_y = Align::TOP;
}

void HBox::__init__(const rect &r) {
	new(this) HBox(r);
}



VBox::VBox(const rect &r) : Node(r) {
	type = Type::VBOX;
	content_align_x = Align::LEFT;
	content_align_y = Align::FILL;
}

void VBox::__init__(const rect &r) {
	new(this) VBox(r);
}


nix::Shader *shader = nullptr;
nix::VertexBuffer *vertex_buffer = nullptr;
Array<Node*> all_nodes;
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
	for (auto *p: all_nodes) {
		//p->rebuild();
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
