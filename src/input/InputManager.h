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
		for (auto s: ecs::SystemManager::systems) \
			s->NAME(); \
		gui::handle_input(mouse, [](gui::Node *n) { n->NAME(mouse); }); \
	}

#define SEND_EVENT_P(NAME, k) \
	for (auto s: ecs::SystemManager::systems) \
		s->NAME(k);



}
