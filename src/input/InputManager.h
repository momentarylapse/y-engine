/*
 * InputManager.h
 *
 *  Created on: 08.01.2020
 *      Author: michi
 */

#pragma once


class vec3;
struct GLFWwindow;

namespace input {

class Gamepad;

void init(GLFWwindow *window);
void remove(GLFWwindow *window);

void iterate();



#define SEND_EVENT(NAME) \
	{ \
		for (auto *c: ControllerManager::controllers) \
			c->NAME(); \
		gui::handle_input(mouse01, [](gui::Node *n) { return n->NAME(); }); \
	}

#define SEND_EVENT_P(NAME, k) \
	for (auto *c: ControllerManager::controllers) \
		c->NAME(k);



}
