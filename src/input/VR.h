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

	struct VRDevice {
		void* object;
		int type;
		string name;
		vec3 pos;
		quaternion ang;
	};

	VRDevice* get_vr_device(int index);


}
