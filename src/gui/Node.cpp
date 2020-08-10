/*
 * Layer.cpp
 *
 *  Created on: Aug 10, 2020
 *      Author: michi
 */

#include "Node.h"
#include "gui.h"
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

}
