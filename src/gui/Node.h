/*
 * Layer.h
 *
 *  Created on: Aug 10, 2020
 *      Author: michi
 */

#ifndef SRC_GUI_NODE_H_
#define SRC_GUI_NODE_H_

//#include "../lib/vulkan/vulkan.h"
#include "../lib/nix/nix.h"

namespace gui {

enum class Type {
	NODE,
	HBOX,
	VBOX,
	PICTURE,
	TEXT,
	MODEL
};

class Node : public VirtualBase {
public:
	Node(const rect &r);
	virtual ~Node();

	void __init__(const rect &r);
	virtual void __delete__();

	enum class Align {
		NONE,
		FILL,
		TOP,
		BOTTOM,
		LEFT = TOP,
		RIGHT = BOTTOM
	};

	Type type;
	bool visible;
	rect area;
	rect margin;
	//rect padding;
	Align content_align_x, content_align_y;
	color col;
	float dz;

	color eff_col;
	rect eff_area;
	float eff_z;

	Node *parent;
	Array<Node*> children;
	void add(Node *);

	virtual void on_iterate(float dt) {}
	virtual void on_left_button_down() {}
	virtual void on_left_button_up() {}
	virtual void on_right_button_down() {}
	virtual void on_right_button_up() {}

	virtual void on_enter() {}
	virtual void on_leave() {}
};

class HBox : public Node {
public:
	HBox(const rect &r);
	void __init__(const rect &r);
};

class VBox : public Node {
public:
	VBox(const rect &r);
	void __init__(const rect &r);
};

	//void init(vulkan::RenderPass *rp);
	void init(nix::Shader *s);
	void reset();
	//void render(vulkan::CommandBuffer *cb, const rect &viewport);
	void update();

	extern Node* toplevel;
	extern Array<Node*> all_nodes;
	extern nix::Shader *shader;
	extern nix::VertexBuffer *vertex_buffer;
}

#endif /* SRC_GUI_NODE_H_ */
