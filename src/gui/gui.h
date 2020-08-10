/*
 * gui.h
 *
 *  Created on: Aug 11, 2020
 *      Author: michi
 */

#ifndef SRC_GUI_GUI_H_
#define SRC_GUI_GUI_H_


//#include "../lib/vulkan/vulkan.h"
#include "../lib/nix/nix.h"
#include <functional>

namespace gui {

class Node;

//void init(vulkan::RenderPass *rp);
void init(nix::Shader *s);
void reset();
//void render(vulkan::CommandBuffer *cb, const rect &viewport);
void update();
void handle_input(const vector &m, std::function<bool(Node *n)> f);
void handle_mouse_move(const vector &m_prev, const vector &m);
void iterate(float dt);

extern Node* toplevel;
extern Array<Node*> all_nodes;
extern Array<Node*> sorted_nodes;
extern nix::Shader *shader;
extern nix::VertexBuffer *vertex_buffer;
}




#endif /* SRC_GUI_GUI_H_ */
