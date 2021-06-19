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


class Gamepad: public Sharable<Empty> {
public:
	enum class BUTTON {
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
	void update();
	bool clicked(BUTTON b);
};


void iterate_pads();

shared<Gamepad> get_pad(int index);

}
