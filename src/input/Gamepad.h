/*
 * Gamepad.h
 *
 *  Created on: Jun 19, 2021
 *      Author: michi
 */

#pragma once

#include "../lib/base/pointer.h"

struct GLFWgamepadstate;

namespace input {

/*class GamepadState {
public:
	bool buttons[16] = {false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false};
	float axes[6] = {0, 0, 0, 0, 0, 0};
};*/


class Gamepad: public Sharable<base::Empty> {
public:
	enum class Button {
		CROSS,
		CIRCLE,
		SQUARE,
		TRIANGLE,
		L1,
		R1,
		OPTIONS,
		SHARE,
		PS,
		L3,
		R3,
		UP,
		RIGHT,
		DOWN,
		LEFT
	};


	owned<GLFWgamepadstate> state;
	owned<GLFWgamepadstate> state_prev;
	int index;
	float deadzone = 0.05f;

	Gamepad(int index);
	~Gamepad();

	bool is_present() const;
	string name() const;
	void update();
	float axis(int i);
	bool button(Button b);
	bool clicked(Button b);
};


void init_pads();
void iterate_pads();
shared<Gamepad> get_pad(int index);

extern bool link_mouse_and_keyboard_into_pad;

}
