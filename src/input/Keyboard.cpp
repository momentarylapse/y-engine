/*
 * Keyboard.cpp
 *
 *  Created on: Jun 19, 2021
 *      Author: michi
 */

#include "Keyboard.h"
#include "InputManager.h"
#include "../lib/hui_minimal/hui.h"
#include "../plugins/PluginManager.h"
#include "../plugins/Controller.h"
#include <GLFW/glfw3.h>

namespace input {


struct KeyboardState {
	bool key[256];
	void reset() {
		for (int i=0; i<256; i++)
			key[i] = false;
	}
};

KeyboardState keyboard_state;
KeyboardState keyboard_state_prev;



void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);


void init_keyboard(GLFWwindow *window) {
	keyboard_state.reset();
	keyboard_state_prev.reset();

	glfwSetKeyCallback(window, key_callback);
}

void remove_keyboard(GLFWwindow *window) {
	glfwSetKeyCallback(window, nullptr);
}

void iterate_keyboard_pre() {
	keyboard_state_prev = keyboard_state;
}

void iterate_keyboard() {}

bool get_key(int k) {
	if (k == hui::KEY_SHIFT)
		return keyboard_state.key[hui::KEY_RSHIFT] or keyboard_state.key[hui::KEY_LSHIFT];
	if (k == hui::KEY_CONTROL)
		return keyboard_state.key[hui::KEY_RCONTROL] or keyboard_state.key[hui::KEY_LCONTROL];
	if (k < 0 or k >= 256)
		return false;
	return keyboard_state.key[k];
}

bool get_key_down(int k) {
	if (k < 0 or k >= 256)
		return false;
	return keyboard_state.key[k] and !keyboard_state_prev.key[k];
}

bool get_key_up(int k) {
	if (k < 0 or k >= 256)
		return false;
	return !keyboard_state.key[k] and keyboard_state_prev.key[k];
}

int key_decode(int key) {
	for (int i=0; i<10; i++)
		if (key == GLFW_KEY_0 + i)
			return hui::KEY_0 + i;
	for (int i=0; i<26; i++)
		if (key == GLFW_KEY_A + i)
			return hui::KEY_A + i;
	for (int i=0; i<12; i++)
		if (key == GLFW_KEY_F1 + i)
			return hui::KEY_F1 + i;
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
		return hui::KEY_PAGE_UP;
	if (key == GLFW_KEY_PAGE_DOWN)
		return hui::KEY_PAGE_DOWN;
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
	if (key == GLFW_KEY_ESCAPE)
		return hui::KEY_ESCAPE;
	return -1;
}

int mods_decode(int mods) {
	int r = 0;
	if (mods == GLFW_MOD_SHIFT)
		r += hui::KEY_SHIFT;
	if (mods == GLFW_MOD_CONTROL)
		r += hui::KEY_CONTROL;
	if (mods == GLFW_MOD_ALT)
		r += hui::KEY_ALT;
	//if (mods == GLFW_MOD_SUPER)
	//	r += hui::KEY_SUPER;
	return r;
}


void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
	int k = key_decode(key);
	if (k < 0)
		return;

	if (action == GLFW_PRESS) {
		keyboard_state.key[k] = true;
	} if (action == GLFW_RELEASE) {
		keyboard_state.key[k] = false;
	}

	k += mods_decode(mods);
	//std::cout << "key " << k << "    " << key << " " << action << " " << mods << "\n";

	if (action == GLFW_PRESS) {
		SEND_EVENT_P(on_key_down, k);
	} if (action == GLFW_RELEASE) {
		SEND_EVENT_P(on_key_up, k);
	}
}

}
