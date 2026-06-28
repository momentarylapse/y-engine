/*
 * Layer.cpp
 *
 *  Created on: Aug 10, 2020
 *      Author: michi
 */

#include "Node.h"
#include "gui.h"
#include <EngineData.h>
#include <lib/layout/Resource.h>
#include <lib/layout/Node.h>
#include <lib/yrenderer/Context.h>
#include <lib/ygraphics/graphics-impl.h>
#include "Text.h"
#include "lib/os/msg.h"
//#include <algorithm>

namespace gui {

Node::Node() : Node(rect::ID) {}

Node::Node(const rect &r) {
	type = Type::NODE;
	id = p2s(this);
	set_area(r);
	dz = 1;
	col = White;
	visible = true;
	margin = rect::EMPTY;
	size_mode_x = layout::SizeMode::Shrink;
	size_mode_y = layout::SizeMode::Shrink;
	align = {0, 0};
	non_square = false;

	eff_col = White;
	eff_area = r;
	eff_z = 0;
	eff_visible = true;

	parent = nullptr;
}

Node::~Node() = default;

void Node::add(shared<Node> n) {
	children.add(n);
	n->parent = this;
	update_tree();
}

void Node::remove(Node &n) {
	for (int i=0; i<children.num; i++)
		if (children[i] == &n)
			children.erase(i);
	update_tree();
}

void Node::remove_all_children() {
	children.clear();
	update_tree();
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

void Node::set_area(const rect &r) {
	pos.x = r.x1;
	pos.y = r.y1;
	width = r.width();
	height = r.height();
}

void Node::update_geometry(const rect &target) {
	if (parent) {
		eff_z = parent->eff_z + dz;
		eff_col = col;//parent->eff_col * col;
		eff_visible = parent->eff_visible and visible;
	} else {
		// toplevel
		eff_col = col;
		eff_z = 0;
		eff_visible = visible;
	}

	if (parent) {
		eff_area = target;
		float fx = 1/engine.physical_aspect_ratio;
		if (non_square)
			fx = 1;

		if (size_mode_x == layout::SizeMode::Shrink) {
			eff_area.x1 = target.x1 + align.x * target.width() + (margin.x1 + pos.x - align.x * (margin.x1 + margin.x2 + width)) * fx;
			eff_area.x2 = eff_area.x1 + width * fx;
		} else {
			eff_area.x1 = target.x1 + margin.x1 * fx;
			eff_area.x2 = target.x2 - margin.x2 * fx;
		}

		if (size_mode_y == layout::SizeMode::Shrink) {
			eff_area.y1 = target.y1 + align.y * target.height() + (margin.y1 + pos.y - align.y * (margin.y1 + margin.y2 + height));
			eff_area.y2 = eff_area.y1 + height;
		} else {
			eff_area.y1 = target.y1 + margin.y1;
			eff_area.y2 = target.y2 - margin.y2;
		}

		//eff_area = rect_sub_margin(eff_area, margin);

	} else {
		// toplevel
		eff_area = target;
	}

	auto sub_area = eff_area;
	for (auto n: weak(children)) {
		n->update_geometry(sub_area);
		if (type == Type::VBOX)
			sub_area.y1 = n->eff_area.y2 + n->margin.y2;
		if (type == Type::HBOX)
			sub_area.x1 = n->eff_area.x2 + n->margin.x2;
	}
}

Node* Node::get(const string& _id) {
	if (id == _id)
		return this;
	for (auto n: weak(children)) {
		if (n->id == _id)
			return n;
		if (auto c = n->get(_id))
			return c;
	}
	return nullptr;
}

void print_resource(const layout::Resource& r, const string& prefix) {
	msg_write(format("%s%s %s  %s", prefix, r.type, r.id, str(r.options)));
	for (const auto& c: r.children)
		print_resource(c, prefix + "    ");
}

void Node::apply_resource(const layout::Resource &r) {
	if (r.id != "?" and r.id != "")
		id = r.id;
	//n->align = Align::NONE;
	for (const auto& o: r.options)
		set_option(o.key, o.value);

	for (const auto& c: r.children) {
		if (Node* n = create_node(c.type)) {
			//msg_write("create " + c.type + "   " + p2s(n));
			add(n);
			n->apply_resource(c);
		}
	}
}


void Node::add_from_source(const string& source) {
	auto r = layout::Resource::parse(source, false);
	print_resource(r, "");
	//if (r.id != "?")

	apply_resource(r);
}

void Node::_set_option(const string& key, const string& value) {
	if (key == "width") {
		width = value._float();
	} else if (key == "height") {
		height = value._float();
	} else if (key == "x") {
		pos.x = value._float();
	} else if (key == "y") {
		pos.y = value._float();
	} else if (key == "dz") {
		dz = value._float();
	} else if (key == "margin") {
		float f = value._float();
		margin = rect(f, f, f, f);
	} else if (key == "visible") {
		visible = value._bool();
	} else if (key == "hidden") {
		visible = false;
	} else if (key == "color") {
		col = color::parse(value);
	} else if (key == "top") {
		size_mode_y = layout::SizeMode::Shrink;
		align.y = 0;
	} else if (key == "bottom") {
		size_mode_y = layout::SizeMode::Shrink;
		align.y = 1;
	} else if (key == "left") {
		size_mode_x = layout::SizeMode::Shrink;
		align.x = 0;
	} else if (key == "right") {
		size_mode_x = layout::SizeMode::Shrink;
		align.x = 1;
	} else if (key == "centerh") {
		size_mode_x = layout::SizeMode::Shrink;
		align.x = 0.5f;
	} else if (key == "centerv") {
		size_mode_y = layout::SizeMode::Shrink;
		align.y = 0.5f;
	} else if (key == "center") {
		size_mode_x = layout::SizeMode::Shrink;
		size_mode_y = layout::SizeMode::Shrink;
		align = {0.5f, 0.5f};
	} else if (key == "fillx") {
		size_mode_x = layout::SizeMode::Fill;
	} else if (key == "filly") {
		size_mode_y = layout::SizeMode::Fill;
	} else if (key == "nonsquare") {
		non_square = true;
	}
}

void Node::set_option(const string& key, const string& value) {
	if (type == Type::TEXT)
		(static_cast<Text&>(*this)._set_option(key, value));
	else if (type == Type::PICTURE)
		(static_cast<Picture&>(*this)._set_option(key, value));
	else
		_set_option(key, value);
}


HBox::HBox() {
	type = Type::HBOX;
	size_mode_x = layout::SizeMode::Fill;
	size_mode_y = layout::SizeMode::Fill;
}



VBox::VBox() {
	type = Type::VBOX;
	size_mode_x = layout::SizeMode::Fill;
	size_mode_y = layout::SizeMode::Fill;
}

}
