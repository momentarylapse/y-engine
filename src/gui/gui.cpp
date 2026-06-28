/*
 * gui.cpp
 *
 *  Created on: Aug 11, 2020
 *      Author: michi
 */

#include "gui.h"
#include "Node.h"
#include "Font.h"
#include "Text.h"
#include "Canvas.h"
#include <lib/math/rect.h>
#include <lib/kaba/kaba.h>
#include <lib/layout/Node.h>
#include <lib/profiler/Profiler.h>
#include "../plugins/PluginManager.h"
#include "lib/os/msg.h"


extern bool _parse_tokens_smart_strings_;

namespace gui {

Array<Node*> all_nodes;
Array<Node*> sorted_nodes;
shared<Node> toplevel;
static int ch_gui_iter = -1;

ygfx::DrawingHelperData* aux = nullptr;
float ui_scale;


void init(int ch_iter) {
	ch_gui_iter = profiler::create_channel("gui", ch_iter);

	init_fonts();

	toplevel = new Node();
	toplevel->size_mode_x = layout::SizeMode::Fill;
	toplevel->size_mode_y = layout::SizeMode::Fill;
}

void reset() {
	toplevel = new Node();
	toplevel->size_mode_x = layout::SizeMode::Fill;
	toplevel->size_mode_y = layout::SizeMode::Fill;

	all_nodes = {};
	sorted_nodes = {};
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
	//update();
}

void update(float aspect_ratio) {
	if (toplevel)
		toplevel->negotiate_outer_area({0,aspect_ratio, 0,1});

	sorted_nodes = all_nodes;
	return;
	//std::sort(sorted_nodes.begin(), sorted_nodes.end(), [](Node *a, Node *b) { return a->eff_z < b->eff_z; });
	for (int i=0; i<sorted_nodes.num; i++)
		for (int j=i+1; j<sorted_nodes.num; j++)
			if (sorted_nodes[i]->eff_z > sorted_nodes[j]->eff_z)
				sorted_nodes.swap(i, j);


	//for (auto *p: all_nodes) {
	//	p->rebuild();
	//}
}

// input: [0:R]x[0:1]
void handle_input(const vec2 &m, std::function<void(Node *n)> f) {
	foreachb(Node *n, sorted_nodes) {
		if (n->allow_hover and n->visible and n->area.inside(m)) {
			f(n);
			return;
		}
	}
}

// input: [0:R]x[0:1]
void handle_mouse_move(const vec2 &m_prev, const vec2 &m) {
	for (auto n: all_nodes) {
		if (n->area.inside(m) and !n->area.inside(m_prev))
			n->on_enter(m);
		if (!n->area.inside(m) and n->area.inside(m_prev))
			n->on_leave();
	}
}

void iterate(float dt) {
	profiler::begin(ch_gui_iter);
	auto nodes = all_nodes;
	// tree might change...
	for (auto n: nodes) {
		if (n->visible) {
#if 0
			[[maybe_unused]] auto prev = std::chrono::high_resolution_clock::now();
#endif
			n->on_iterate(dt);
#if 0
			auto now = std::chrono::high_resolution_clock::now();
			float dt = std::chrono::duration<float, std::chrono::seconds::period>(now - prev).count();
			auto cc = kaba::get_dynamic_type(n);
			if (cc)
				printf("%s     %.2f\n", cc->long_name().c_str(), dt * 1000.0f);
			else
				printf("????     %.2f\n", dt * 1000.0f);
#endif
		}
	}
	profiler::end(ch_gui_iter);
}

void delete_node(Node *n) {
	if (n->parent) {
		for (int i=0; i<n->parent->children.num; i++)
			if (n == n->parent->children[i])
				n->parent->children.erase(i);
	}
	update_tree();
}


Node* create_node(const string& type) {
	if (type == "Node")
		return new Node();
	if (type == "Picture" or type == "Rectangle")
		return new Picture();
	if (type == "Text")
		return new Text();
	if (type == "Canvas")
		return new Canvas();
	if (type == "HBox")
		return new HBox();
	if (type == "VBox")
		return new VBox();
	if (type.find(".") >= 0)
		return static_cast<Node*>(PluginManager::create_instance_auto(type));
	return nullptr;
}


}
