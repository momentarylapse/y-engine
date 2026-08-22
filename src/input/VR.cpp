//
// Created by michi on 01.05.25.
//

#include "VR.h"
#include <lib/os/msg.h>
#include <lib/vr/vr.h>


namespace input {

bool vr_active = false;

#if HAS_LIB_OPENXR

static Array<VRDevice> vr_devices;

bool VRDevice::button(int index) const {
	return (button_mask & (1 << index)) != 0;
}

bool VRDevice::clicked(int index) const {
	return ((button_mask & (1 << index)) != 0) and !(button_mask_prev & (1 << index));
}


float VRDevice::axis(int index) const {
	if (index < 0 or index >= 12)
		return 0;
	return _axis[index];
}

void init_vr() {
	vr_devices.add(VRDevice(nullptr, VRDeviceRole::ControllerLeft, "left"));
	vr_devices.add(VRDevice(nullptr, VRDeviceRole::ControllerRight, "right"));
	vr_active = true;
}

void iterate_vr() {
	if (!vr_active)
		return;

	for (int i=0; i<2; i++) {
		const auto& c = vr::instance->controllers[i];
		auto cc = get_vr_device(i == 1 ? VRDeviceRole::ControllerRight : VRDeviceRole::ControllerLeft);
		cc->pos = c.pos;
		cc->ang = c.ang;
		cc->button_mask = c.button_a << 4 | c.button_b << 5;
		if (c.trigger > 0.5f)
			cc->button_mask |= 1 << 0;
		cc->_axis[1] = c.trigger;
		cc->_axis[10] = c.thumb_stick.x;
		cc->_axis[11] = c.thumb_stick.y;
	}

}

VRDevice* get_vr_device(VRDeviceRole role) {
	for (auto& d: vr_devices)
		if (d.role == role)
			return &d;
	return nullptr;
}


#else

void init_vr() {}

void iterate_vr() {}

VRDevice* get_vr_device(VRDeviceRole role) {
	return nullptr;
}

float VRDevice::axis(int index) const {
	return 0;
}

bool VRDevice::button(int b) const {
	return false;
}

bool VRDevice::clicked(int b) const {
	return false;
}




#endif
}
