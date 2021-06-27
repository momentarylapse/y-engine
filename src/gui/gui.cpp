/*
 * gui.cpp
 *
 *  Created on: Aug 11, 2020
 *      Author: michi
 */

#include "gui.h"
#include "Node.h"
#include "Font.h"
#include "../meta.h"
#include "../lib/math/rect.h"
#include "../lib/math/vector.h"
#include "../lib/math/matrix.h"
#include "../lib/nix/nix.h"
#include <stdio.h>

namespace gui {

shared<nix::Shader> shader;
nix::VertexBuffer *vertex_buffer = nullptr;
Array<Node*> all_nodes;
Array<Node*> sorted_nodes;
shared<Node> toplevel;


void init(nix::Shader *s) {
	vertex_buffer = new nix::VertexBuffer("3f,3f,2f|i");
	vertex_buffer->create_rect(rect::ID);

	Font::init_fonts();

	shader = s;
	toplevel = new Node(rect::ID);
}

void reset() {
	toplevel = new Node(rect::ID);
}

void add_to_node_list(Node *n) {
	all_nodes.append(weak(n->children));
	for (auto c: weak(n->children))
		add_to_node_list(c);
}

void update_tree() {
	all_nodes.clear();
	if (toplevel)
		add_to_node_list(toplevel.get());
	update();
}

void update() {
	if (toplevel)
		toplevel->update_geometry(rect::ID);

	sorted_nodes = all_nodes;
	//std::sort(sorted_nodes.begin(), sorted_nodes.end(), [](Node *a, Node *b) { return a->eff_z < b->eff_z; });
	for (int i=0; i<sorted_nodes.num; i++)
		for (int j=i+1; j<sorted_nodes.num; j++)
			if (sorted_nodes[i]->eff_z > sorted_nodes[j]->eff_z)
				sorted_nodes.swap(i, j);


	for (auto *p: all_nodes) {
		//p->rebuild();
	}
}

// input: [0:1]x[0:1]
void handle_input(const vector &m, std::function<bool(Node *n)> f) {
	foreachb(Node *n, sorted_nodes) {
		if (n->eff_area.inside(m.x, m.y)) {
			if (f(n))
				return;
		}
	}
}

// input: [0:1]x[0:1]
void handle_mouse_move(const vector &m_prev, const vector &m) {
	for (auto n: all_nodes) {
		if (n->eff_area.inside(m.x, m.y) and !n->eff_area.inside(m_prev.x, m_prev.y))
			n->on_enter();
		if (!n->eff_area.inside(m.x, m.y) and n->eff_area.inside(m_prev.x, m_prev.y))
			n->on_leave();
	}
}

void iterate(float dt) {
	auto nodes = all_nodes;
	// tree might change...
	for (auto n: nodes) {
		n->on_iterate(dt);
	}
}

void delete_node(Node *n) {
	if (n->parent) {
		for (int i=0; i<n->parent->children.num; i++)
			if (n == n->parent->children[i])
				n->parent->children.erase(i);
	}
	update_tree();
}


}
