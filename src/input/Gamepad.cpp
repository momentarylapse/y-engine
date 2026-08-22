/*
 * Gamepad.cpp
 *
 *  Created on: Jun 19, 2021
 *      Author: michi
 */

#include "Gamepad.h"
#include "Mouse.h"
#include "Keyboard.h"
#include "VR.h"
#include <EngineData.h>
#include <cmath>
#include <GLFW/glfw3.h>
#include <lib/math/vec2.h>
#include <lib/os/msg.h>

namespace input {

bool link_mouse_and_keyboard_into_pad = true;

shared_array<Gamepad> gamepads;
shared<Gamepad> main_pad;

Gamepad::Gamepad(int _index) {
	index = _index;
	state = new GLFWgamepadstate;
	state_prev = new GLFWgamepadstate;
}

Gamepad::~Gamepad() = default;

bool Gamepad::is_present() const {
	return glfwJoystickIsGamepad(index) == 1;
}

string Gamepad::name() const {
	return glfwGetGamepadName(index);
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


float Gamepad::axis(int i) {
	return state->axes[i];
}

bool Gamepad::button(Button b) {
	return state->buttons[(int)b];
}

bool Gamepad::clicked(Button b) {
	return state->buttons[(int)b] and not state_prev->buttons[(int)b];
}


void init_pads() {
	main_pad = get_pad(-1);
}

void iterate_pads() {
	for (auto pad: weak(gamepads))
		pad->update();

	if (link_mouse_and_keyboard_into_pad) {
		if ((dmouse.x != 0) and (engine.elapsed_rt != 0))
			main_pad->state->axes[2] = dmouse.x / engine.elapsed_rt * 0.5f;
		if ((dmouse.y != 0) and (engine.elapsed_rt != 0))
			main_pad->state->axes[3] = dmouse.y / engine.elapsed_rt * 0.5f;
		if (get_button(0))
			main_pad->state->buttons[(int)Gamepad::Button::R1] = true;
		if (get_key(KEY_UP) or get_key(KEY_W))
			main_pad->state->axes[1] = -1;
		if (get_key(KEY_DOWN) or get_key(KEY_S))
			main_pad->state->axes[1] = 1;
		if (get_key(KEY_RIGHT) or get_key(KEY_D))
			main_pad->state->axes[0] = 1;
		if (get_key(KEY_LEFT) or get_key(KEY_A))
			main_pad->state->axes[0] = -1;

		if (vr_active) {
			if (auto c = get_vr_device(VrDeviceRole::ControllerLeft)) {
				main_pad->state->axes[0] = c->axis((int)VrAxis::JOYSTICK_H);
				main_pad->state->axes[1] = -c->axis((int)VrAxis::JOYSTICK_V);
				main_pad->state->axes[4] = c->axis((int)VrAxis::TRIGGER);
				main_pad->state->buttons[(int)Gamepad::Button::UP] = c->button((int)VrButton::A);
				main_pad->state->buttons[(int)Gamepad::Button::Down] = c->button((int)VrButton::B);
			}
			if (auto c = get_vr_device(VrDeviceRole::ControllerRight)) {
				main_pad->state->axes[2] = c->axis((int)VrAxis::JOYSTICK_H);
				main_pad->state->axes[3] = -c->axis((int)VrAxis::JOYSTICK_V);
				main_pad->state->axes[5] = c->axis((int)VrAxis::TRIGGER);
				main_pad->state->buttons[(int)Gamepad::Button::Triangle] = c->button((int)VrButton::A);
				main_pad->state->buttons[(int)Gamepad::Button::Cross] = c->button((int)VrButton::B);
			}
		}
	}
}

int find_best_pad() {
	for (int i=0; i<16; i++)
		if (glfwJoystickIsGamepad(i))
			return i;
	return 0;
}

shared<Gamepad> get_pad(int index) {
	if (index < 0)
		index = find_best_pad();
	for (auto pad: weak(gamepads))
		if (pad->index == index)
			return pad;
	auto pad = new Gamepad(index);
	gamepads.add(pad);
	return pad;
}

}
