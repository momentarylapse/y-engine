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
#include <functional>

namespace gui {

class Node : public VirtualBase {
public:
	Node(const rect &r);
	virtual ~Node();

	void __init__(const rect &r);
	virtual void __delete__();


	enum class Type {
		NODE,
		HBOX,
		VBOX,
		PICTURE,
		TEXT,
		MODEL
	};

	enum Align {
		NONE,
		FILL_X = 1<<0,
		FILL_Y = 1<<1,
		TOP = 1<<2,
		BOTTOM = 1<<3,
		LEFT = 1<<4,
		RIGHT = 1<<5,
		_FILL_XY = FILL_X | FILL_Y,
		_TOP_LEFT = TOP | LEFT,
	};

	Type type;
	bool visible;
	rect area;
	rect margin;
	//rect padding;
	Align align;
	color col;
	//float group_alpha;
	float dz;

	color eff_col;
	rect eff_area;
	float eff_z;

	Node *parent;
	Array<Node*> children;
	void add(Node *);
	void update_geometry(const rect &target);

	virtual void on_iterate(float dt) {}
	virtual bool on_left_button_down() { return false; }
	virtual bool on_left_button_up() { return false; }
	virtual bool on_middle_button_down() { return false; }
	virtual bool on_middle_button_up() { return false; }
	virtual bool on_right_button_down() { return false; }
	virtual bool on_right_button_up() { return false; }

	virtual void on_enter() {}
	virtual void on_leave() {}
};

class HBox : public Node {
public:
	HBox();
	void __init__();
};

class VBox : public Node {
public:
	VBox();
	void __init__();
};

/*class Spacer : public Node {
public:
	Spacer(float w, float h);
	void __init__(float w, float h);
};


class Layer : public Node {
public:
	Layer(const rect &r);
	void __init__(const rect &r);
};*/

//void init(vulkan::RenderPass *rp);
void init(nix::Shader *s);
void reset();
//void render(vulkan::CommandBuffer *cb, const rect &viewport);
void update();
void handle_input(float mx, float my, std::function<bool(Node *n)> f);
void iterate(float dt);

extern Node* toplevel;
extern Array<Node*> all_nodes;
extern Array<Node*> sorted_nodes;
extern nix::Shader *shader;
extern nix::VertexBuffer *vertex_buffer;
}

#endif /* SRC_GUI_NODE_H_ */
