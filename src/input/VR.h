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

	enum class VRDeviceRole {
		None = -1,
		ControllerRight,
		ControllerLeft,
		Headset,
		Lighthouse0,
		Lighthouse1
	};

	struct VRDevice {
		void* object;
		VRDeviceRole role;
		string name;
		vec3 pos;
		quaternion ang;
		int button_mask = 0;
		int button_mask_prev = 0;
		bool button(int b) const;
		bool clicked(int b) const;
		float axis(int index) const;
	};

	VRDevice* get_vr_device(VRDeviceRole role);


}
