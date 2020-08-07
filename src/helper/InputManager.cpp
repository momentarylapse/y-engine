/*
 * InputManager.cpp
 *
 *  Created on: 08.01.2020
 *      Author: michi
 */

#include "InputManager.h"
#include "../plugins/PluginManager.h"
#include "../plugins/Controller.h"
#include "../lib/hui/hui.h"
#include "../lib/math/math.h"
#include <iostream>

InputManager::State InputManager::state;
InputManager::State InputManager::state_prev;
vector InputManager::mouse;
vector InputManager::dmouse;
vector InputManager::scroll;

void InputManager::State::reset() {
	for (int i=0; i<256; i++)
		key[i] = false;
	for (int i=0; i<3; i++)
		button[i] = false;
	mx = my = 0;
	dx = dy = 0;

	scroll_x = scroll_y = 0;
}


void InputManager::init(GLFWwindow *window) {
	state.reset();
	state_prev.reset();
	mouse = dmouse = scroll = v_0;

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	if (glfwRawMouseMotionSupported())
		glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

	// init with current position (to avoid dmouse jumps)
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);
	state_prev.mx = state.mx = xpos;
	state_prev.my = state.my = ypos;


	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetScrollCallback(window, scroll_callback);
}

void InputManager::iterate() {
	state.dx = state.mx - state_prev.mx;
	state.dy = state.my - state_prev.my;
	state_prev = state;

	glfwPollEvents();

	mouse = vector(clampf(state.mx/1000.0f, 0, 1), clampf(state.my/1000.0f, 0, 1), 0);
	dmouse = vector(state.dx, state.dy, 0) / 1000.0f;
	scroll = vector(state.scroll_x, state.scroll_y, 0);

	state.scroll_x = state.scroll_y = 0;
}

bool InputManager::get_key(int k) {
	if (k == hui::KEY_SHIFT)
		return state.key[hui::KEY_RSHIFT] or state.key[hui::KEY_LSHIFT];
	if (k == hui::KEY_CONTROL)
		return state.key[hui::KEY_RCONTROL] or state.key[hui::KEY_LCONTROL];
	if (k < 0 or k >= 256)
		return false;
	return state.key[k];
}

bool InputManager::get_key_down(int k) {
	if (k < 0 or k >= 256)
		return false;
	return state.key[k] and !state_prev.key[k];
}

bool InputManager::get_key_up(int k) {
	if (k < 0 or k >= 256)
		return false;
	return !state.key[k] and state_prev.key[k];
}

int key_decode(int key) {
	for (int i=0; i<10; i++)
		if (key == GLFW_KEY_0 + i)
			return hui::KEY_0 + i;
	for (int i=0; i<26; i++)
		if (key == GLFW_KEY_A + i)
			return hui::KEY_A + i;
	if (key == GLFW_KEY_ENTER)
				return hui::KEY_RETURN;
	if (key == GLFW_KEY_SPACE)
				return hui::KEY_SPACE;
	if (key == GLFW_KEY_BACKSPACE)
				return hui::KEY_BACKSPACE;
	if (key == GLFW_KEY_UP)
				return hui::KEY_UP;
	if (key == GLFW_KEY_DOWN)
				return hui::KEY_DOWN;
	if (key == GLFW_KEY_LEFT)
				return hui::KEY_LEFT;
	if (key == GLFW_KEY_RIGHT)
				return hui::KEY_RIGHT;
	if (key == GLFW_KEY_LEFT_SHIFT)
				return hui::KEY_LSHIFT;
	if (key == GLFW_KEY_RIGHT_SHIFT)
				return hui::KEY_RSHIFT;
	if (key == GLFW_KEY_LEFT_CONTROL)
				return hui::KEY_LCONTROL;
	if (key == GLFW_KEY_RIGHT_CONTROL)
				return hui::KEY_RCONTROL;
	if (key == GLFW_KEY_PAGE_UP)
				return hui::KEY_PRIOR;
	if (key == GLFW_KEY_PAGE_DOWN)
				return hui::KEY_NEXT;
	if (key == GLFW_KEY_HOME)
				return hui::KEY_HOME;
	if (key == GLFW_KEY_END)
				return hui::KEY_END;
	if (key == GLFW_KEY_DELETE)
				return hui::KEY_DELETE;
	if (key == GLFW_KEY_INSERT)
				return hui::KEY_INSERT;
	if (key == GLFW_KEY_TAB)
				return hui::KEY_TAB;
	return -1;
}

int mods_decode(int mods) {
	int r = 0;
	if (mods == 1)
		r += hui::KEY_SHIFT;
	if (mods == 2)
		r += hui::KEY_SHIFT;
	if (mods == 4)
		r += hui::KEY_ALT;
	//if (mods == 8)
	//	r += hui::KEY_SUPER;
	return r;
}

#define SEND_EVENT(NAME) \
	for (auto *c: plugin_manager.controllers) \
		c->NAME();

#define SEND_EVENT_P(NAME, k) \
	for (auto *c: plugin_manager.controllers) \
		c->NAME(k);

void InputManager::key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
	int k = key_decode(key);
	if (k < 0)
		return;

	if (action == GLFW_PRESS) {
		state.key[k] = true;
	} if (action == GLFW_RELEASE) {
		state.key[k] = false;
	}

	k += mods_decode(mods);
	//std::cout << "key " << k << "    " << key << " " << action << " " << mods << "\n";

	if (action == GLFW_PRESS) {
		SEND_EVENT_P(on_key_down, k);
	} if (action == GLFW_RELEASE) {
		SEND_EVENT_P(on_key_up, k);
	}
}

void InputManager::cursor_position_callback(GLFWwindow *window, double xpos, double ypos) {
	//std::cout << "mouse " << xpos << " " << ypos << "\n";
	state.mx = xpos;
	state.my = ypos;

	//SEND_EVENT(on_mouse_move);
}

void InputManager::mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
	//std::cout << "button " << button << " " << action << " " << mods << "\n";
	if (action == GLFW_PRESS) {
		if (button == GLFW_MOUSE_BUTTON_LEFT)
			SEND_EVENT(on_left_button_down);
		if (button == GLFW_MOUSE_BUTTON_MIDDLE)
			SEND_EVENT(on_middle_button_down);
		if (button == GLFW_MOUSE_BUTTON_RIGHT)
			SEND_EVENT(on_right_button_down);
	} else if (action == GLFW_RELEASE) {
		if (button == GLFW_MOUSE_BUTTON_LEFT)
			SEND_EVENT(on_left_button_up);
		if (button == GLFW_MOUSE_BUTTON_MIDDLE)
			SEND_EVENT(on_middle_button_up);
		if (button == GLFW_MOUSE_BUTTON_RIGHT)
			SEND_EVENT(on_right_button_up);
	}
}

void InputManager::scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	//std::cout << "scroll " << xoffset << " " << yoffset << "\n";
	state.scroll_x = xoffset;
	state.scroll_y = yoffset;
}
