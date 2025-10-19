/*
 * Mouse.cpp
 *
 *  Created on: Jun 19, 2021
 *      Author: michi
 */

#include "Mouse.h"
#include "InputManager.h"
#include "../gui/Node.h"
#include "../gui/gui.h"
#include <lib/math/vec2.h>
#include <EngineData.h>
#include <ecs/System.h>
#include <ecs/SystemManager.h>

#include <GLFW/glfw3.h>

namespace input {


vec2 mouse; //   [0:R]x[0:1] coord system
vec2 mouse01; // [0:1]x[0:1] coord system
vec2 dmouse;
vec2 scroll;
bool ignore_velocity;


struct MouseState {
	bool button[3];
	vec2 m, d, scroll;

	void reset() {
		for (int i=0; i<3; i++)
			button[i] = false;
		m = {0,0};
		d = {0,0};
		scroll = {0,0};
	}
};

MouseState mouse_state;
MouseState mouse_state_prev;



void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

void init_mouse(GLFWwindow *window) {
	mouse_state.reset();
	mouse_state_prev.reset();

	dmouse = scroll = {0,0};
	mouse = {engine.physical_aspect_ratio/2, 0.5f};
	mouse01 = {0.5f, 0.5f};
	ignore_velocity = true;

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	if (glfwRawMouseMotionSupported())
		glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

	// init with current position (to avoid dmouse jumps)
	double xpos, ypos;
	glfwPollEvents();
	glfwGetCursorPos(window, &xpos, &ypos);
	mouse_state_prev.m.x = mouse_state.m.x = xpos;
	mouse_state_prev.m.y = mouse_state.m.y = ypos;


	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetScrollCallback(window, scroll_callback);
}

void remove_mouse(GLFWwindow *window) {
	glfwSetCursorPosCallback(window, nullptr);
	glfwSetMouseButtonCallback(window, nullptr);
	glfwSetScrollCallback(window, nullptr);
}


void iterate_mouse_pre() {
	mouse_state.d = mouse_state.m - mouse_state_prev.m;
	mouse_state_prev = mouse_state;
}

void iterate_mouse() {

	auto mouse01_prev = mouse01;

	//mouse = vec3(clampf(state.mx/1000.0f, 0, 1), clampf(state.my/1000.0f, 0, 1), 0);
	dmouse = mouse_state.d / 500.0f;
	mouse += dmouse;
	mouse.x = clamp(mouse.x, 0.0f, engine.physical_aspect_ratio);
	mouse.y = clamp(mouse.y, 0.0f, 1.0f);
	mouse01 = vec2(mouse.x / engine.physical_aspect_ratio, mouse.y);
	scroll = mouse_state.scroll;

	mouse_state.scroll = {0,0};
	if (ignore_velocity) {
		dmouse = {0,0};
		ignore_velocity = false;
	}

	// FIXME might be handled after a click event...!
	gui::handle_mouse_move(mouse01_prev, mouse01);
}


bool get_button(int index) {
	return mouse_state.button[index];
}


void cursor_position_callback(GLFWwindow *window, double xpos, double ypos) {
	//std::cout << "mouse " << xpos << " " << ypos << "\n";
	mouse_state.m.x = xpos;
	mouse_state.m.y = ypos;

	//SEND_EVENT(on_mouse_move);
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
	//std::cout << "button " << button << " " << action << " " << mods << "\n";

	const Array<int> button_map = {0,2,1};

	if (action == GLFW_PRESS) {
		mouse_state.button[button_map[button]] = true;
		if (button == GLFW_MOUSE_BUTTON_LEFT)
			SEND_EVENT(on_left_button_down);
		if (button == GLFW_MOUSE_BUTTON_MIDDLE)
			SEND_EVENT(on_middle_button_down);
		if (button == GLFW_MOUSE_BUTTON_RIGHT)
			SEND_EVENT(on_right_button_down);
	} else if (action == GLFW_RELEASE) {
		mouse_state.button[button_map[button]] = false;
		if (button == GLFW_MOUSE_BUTTON_LEFT)
			SEND_EVENT(on_left_button_up);
		if (button == GLFW_MOUSE_BUTTON_MIDDLE)
			SEND_EVENT(on_middle_button_up);
		if (button == GLFW_MOUSE_BUTTON_RIGHT)
			SEND_EVENT(on_right_button_up);
	}
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	//std::cout << "scroll " << xoffset << " " << yoffset << "\n";
	mouse_state.scroll.x = xoffset;
	mouse_state.scroll.y = yoffset;
}

}
