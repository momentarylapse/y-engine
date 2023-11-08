/*
 * InputManager.cpp
 *
 *  Created on: 08.01.2020
 *      Author: michi
 */

#include "InputManager.h"
#include "Mouse.h"
#include "Keyboard.h"
#include "Gamepad.h"
#include <helper/PerformanceMonitor.h>

#include <GLFW/glfw3.h>


namespace input {

static int channel;



void init(GLFWwindow *window) {
	channel = PerformanceMonitor::create_channel("in");
	init_mouse(window);
	init_keyboard(window);
	init_pads();
}

void iterate() {
	PerformanceMonitor::begin(channel);
	iterate_mouse_pre();
	iterate_keyboard_pre();

	glfwPollEvents();

	iterate_mouse();
	iterate_keyboard();
	iterate_pads();
	PerformanceMonitor::end(channel);
}


}
