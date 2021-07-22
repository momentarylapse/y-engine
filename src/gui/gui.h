/*
 * gui.h
 *
 *  Created on: Aug 11, 2020
 *      Author: michi
 */

#pragma once


#include "../lib/base/pointer.h"
#include <functional>

namespace nix {
	class VertexBuffer;
	class Shader;
}
class vec2;

namespace gui {

class Node;

//void init(vulkan::RenderPass *rp);
void init(nix::Shader *s);
void reset();
//void render(vulkan::CommandBuffer *cb, const rect &viewport);
void update();
void handle_input(const vec2 &m, std::function<bool(Node *n)> f);
void handle_mouse_move(const vec2 &m_prev, const vec2 &m);
void iterate(float dt);

extern shared<Node> toplevel;
extern Array<Node*> all_nodes;
extern Array<Node*> sorted_nodes;
void update_tree();

void delete_node(Node *n);

extern shared<nix::Shader> shader;
extern nix::VertexBuffer *vertex_buffer;
}



