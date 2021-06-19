/*
 * Gamepad.cpp
 *
 *  Created on: Jun 19, 2021
 *      Author: michi
 */

#include "Gamepad.h"
#include <math.h>
#include <GLFW/glfw3.h>

namespace input {

shared_array<Gamepad> gamepads;

Gamepad::Gamepad(int _index) {
	index = _index;
}

Gamepad::~Gamepad() {
}

bool Gamepad::is_present() const {
	return glfwJoystickIsGamepad(index) == 1;
}


void Gamepad::update() {
	*state_prev = *state;
	glfwGetGamepadState(index, state.get());
	for (int i=0; i<=GLFW_GAMEPAD_AXIS_LAST; i++) {
		auto &a = state->axes[i];
		if (fabs(a) < deadzone)
			a = 0;
		else if (a > 0)
			a = (a - deadzone) / (1 - deadzone);
		else
			a = (a + deadzone) / (1 - deadzone);
	}
}

bool Gamepad::clicked(BUTTON b) {
	return state->buttons[(int)b] and not state_prev->buttons[(int)b];
}

void iterate_pads() {
	for (auto pad: weak(gamepads))
		pad->update();
}

shared<Gamepad> get_pad(int index) {
	for (auto pad: weak(gamepads))
		if (pad->index == index)
			return pad;
	auto pad = new Gamepad(index);
	gamepads.add(pad);
	return pad;
}

}
