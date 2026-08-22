//
// Created by michi on 01.05.25.
//

#pragma once

#include <lib/math/vec3.h>
#include <lib/math/quaternion.h>


namespace input {

	extern bool vr_active;

	void init_vr();
	void iterate_vr();

	enum class VrDeviceRole {
		None = -1,
		ControllerRight,
		ControllerLeft,
		Headset,
		Lighthouse0,
		Lighthouse1
	};

	enum class VrButton {
		Trigger = 0,
		Trackpad = 1,
		Joystick = 2,
		System = 3,
		A = 4,
		B = 5,
		Menu = 6,
		Grip = 7,
	};
	enum class VrAxis {
		TRIGGER = 1,
		TRACKPAD_H = 2,
		TRACKPAD_V = 3,
		MIDDLE_FINGER_PROXIMITY = 4,
		RING_FINGER_PROXIMITY = 5,
		PINKY_FINGER_PROXIMITY = 6,
		TRIGGER_FINGER_PROXIMITY = 7,
		GRIP_FORCE = 8,
		TRACKPAD_FORCE = 9,
		JOYSTICK_H = 10,
		JOYSTICK_V = 11,
		COUNT = 12
	};

	struct VrDevice {
		void* object;
		VrDeviceRole role;
		string name;
		vec3 pos;
		quaternion ang;
		vec3 aim_pos;
		quaternion aim_ang;
		float vibration = 0;
		int button_mask = 0;
		int button_mask_prev = 0;
		int touch_mask = 0;
		float _axis[(int)VrAxis::COUNT];
		bool button(int b) const;
		bool clicked(int b) const;
		bool touch(int b) const;
		float axis(int index) const;
	};

	VrDevice* get_vr_device(VrDeviceRole role);


}
