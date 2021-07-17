/*
 * Mouse.cpp
 *
 *  Created on: Jun 19, 2021
 *      Author: michi
 */

#include "Mouse.h"
#include "InputManager.h"
#include "../plugins/PluginManager.h"
#include "../plugins/Controller.h"
#include "../gui/Node.h"
#include "../gui/gui.h"
#include "../lib/hui_minimal/hui.h"
#include "../lib/math/vector.h"
#include "../y/EngineData.h"

#include <GLFW/glfw3.h>

namespace input {


vector mouse; //   [0:R]x[0:1] coord system
vector mouse01; // [0:1]x[0:1] coord system
vector dmouse;
vector scroll;
bool ignore_velocity;


struct MouseState {
	bool button[3];
	float mx, my;
	float dx, dy;
	float scroll_x, scroll_y;

	void reset() {
		for (int i=0; i<3; i++)
			button[i] = false;
		mx = my = 0;
		dx = dy = 0;

		scroll_x = scroll_y = 0;
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

	dmouse = scroll = v_0;
	mouse = vector(engine.physical_aspect_ratio/2, 0.5f, 0);
	mouse01 = vector(0.5f, 0.5f, 0);
	ignore_velocity = true;

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	if (glfwRawMouseMotionSupported())
		glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

	// init with current position (to avoid dmouse jumps)
	double xpos, ypos;
	glfwPollEvents();
	glfwGetCursorPos(window, &xpos, &ypos);
	mouse_state_prev.mx = mouse_state.mx = xpos;
	mouse_state_prev.my = mouse_state.my = ypos;


	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetScrollCallback(window, scroll_callback);
}


void iterate_mouse_pre() {
	mouse_state.dx = mouse_state.mx - mouse_state_prev.mx;
	mouse_state.dy = mouse_state.my - mouse_state_prev.my;
	mouse_state_prev = mouse_state;
}

void iterate_mouse() {

	auto mouse01_prev = mouse01;

	//mouse = vector(clampf(state.mx/1000.0f, 0, 1), clampf(state.my/1000.0f, 0, 1), 0);
	dmouse = vector(mouse_state.dx, mouse_state.dy, 0) / 500.0f;
	mouse += dmouse;
	mouse.x = clamp(mouse.x, 0.0f, engine.physical_aspect_ratio);
	mouse.y = clamp(mouse.y, 0.0f, 1.0f);
	mouse01 = vector(mouse.x / engine.physical_aspect_ratio, mouse.y, 0);
	scroll = vector(mouse_state.scroll_x, mouse_state.scroll_y, 0);

	mouse_state.scroll_x = mouse_state.scroll_y = 0;
	if (ignore_velocity) {
		dmouse = v_0;
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
	mouse_state.mx = xpos;
	mouse_state.my = ypos;

	//SEND_EVENT(on_mouse_move);
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
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

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	//std::cout << "scroll " << xoffset << " " << yoffset << "\n";
	mouse_state.scroll_x = xoffset;
	mouse_state.scroll_y = yoffset;
}

}
