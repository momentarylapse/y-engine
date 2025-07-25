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
#include "VR.h"
#include <lib/profiler/Profiler.h>
#include <GLFW/glfw3.h>


namespace input {

static int channel;

void init(GLFWwindow *window) {
	channel = profiler::create_channel("in");
	init_mouse(window);
	init_keyboard(window);
	init_pads();
	init_vr();
}

void remove(GLFWwindow *window) {
	remove_mouse(window);
	remove_keyboard(window);
	//remove_pads();
}

void iterate() {
	profiler::begin(channel);
	iterate_mouse_pre();
	iterate_keyboard_pre();

	glfwPollEvents();

	iterate_mouse();
	iterate_keyboard();
	iterate_pads();
	iterate_vr();

	profiler::end(channel);
}


}
