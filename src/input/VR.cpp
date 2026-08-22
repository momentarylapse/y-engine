//
// Created by michi on 01.05.25.
//

#include "VR.h"
#include <lib/os/msg.h>
#include <lib/vr/vr.h>
#include <world/components/Camera.h>
#include <ecs/Entity.h>


namespace input {

bool vr_active = false;

#if HAS_LIB_OPENXR

static Array<VrDevice> vr_devices;

bool VrDevice::button(int index) const {
	return (button_mask & (1 << index)) != 0;
}

bool VrDevice::clicked(int index) const {
	return ((button_mask & (1 << index)) != 0) and !(button_mask_prev & (1 << index));
}

bool VrDevice::touch(int index) const {
	return (touch_mask & (1 << index)) != 0;
}


float VrDevice::axis(int index) const {
	if (index < 0 or index >= (int)VrAxis::COUNT)
		return 0;
	return _axis[index];
}

void init_vr() {
	if (!vr::instance)
		return;
	vr_devices.add(VrDevice(nullptr, VrDeviceRole::ControllerLeft, "left"));
	vr_devices.add(VrDevice(nullptr, VrDeviceRole::ControllerRight, "right"));
	vr_devices.add(VrDevice(nullptr, VrDeviceRole::Headset, "head"));
	vr_active = true;
}

void iterate_vr() {
	if (!vr_active)
		return;

	// reference
	vec3 pos0 = v_0;
	quaternion q0 = quaternion::ID;
	if (cam_main) {
		pos0 = cam_main->owner->pos;
		q0 = cam_main->owner->ang;
	}

	for (int i=0; i<2; i++) {
		// left/right hand
		auto& c = vr::instance->controllers[i];
		auto cc = get_vr_device(i == 1 ? VrDeviceRole::ControllerRight : VrDeviceRole::ControllerLeft);
		cc->pos = pos0 + q0 * c.pos;
		cc->ang = q0 * c.ang;
		cc->aim_pos = pos0 + q0 * c.aim_pos;
		cc->aim_ang = q0 * c.aim_ang;
		cc->button_mask = c.button_a << int(VrButton::A)
			| c.button_b << int(VrButton::B)
			| c.button_menu << int(VrButton::Menu);
		if (c.trigger > 0.5f)
			cc->button_mask |= 1 << int(VrButton::Trigger);
		cc->_axis[(int)VrAxis::TRIGGER] = c.trigger;
		cc->_axis[(int)VrAxis::JOYSTICK_H] = c.thumb_stick.x;
		cc->_axis[(int)VrAxis::JOYSTICK_V] = c.thumb_stick.y;
		cc->_axis[(int)VrAxis::TRACKPAD_H] = c.track_pad.x;
		cc->_axis[(int)VrAxis::TRACKPAD_V] = c.track_pad.y;
		c.vibration = cc->vibration; // output!
	}

	{
		// head
		auto cc = get_vr_device(VrDeviceRole::Headset);
		cc->pos = pos0 + q0 * (vr::instance->eye_pos(0) + vr::instance->eye_pos(1)) / 2;
		cc->ang = q0 * vr::instance->eye_ang(0);
		cc->aim_pos = cc->pos;
		cc->aim_ang = cc->ang;
	}

}

VrDevice* get_vr_device(VrDeviceRole role) {
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
