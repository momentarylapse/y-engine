//
// Created by michi on 01.05.25.
//

#include "VR.h"
#include <lib/os/msg.h>

#if HAS_LIB_SURVIVE && HAS_LIB_CNMATRIX
#define SURVIVE_ENABLE_FULL_API
#include <survive_api.h>
#include <survive.h>
#endif


namespace input {

bool vr_active = false;

#if HAS_LIB_SURVIVE && HAS_LIB_CNMATRIX

static SurviveSimpleContext* survive_ctx = nullptr;
static Array<VRDevice> vr_devices;

static void log_fn(SurviveSimpleContext *actx, SurviveLogLevel logLevel, const char *msg) {
	//fprintf(stderr, "(%7.3f) LibSurvive: %s\n", survive_simple_run_time(actx), msg);
}
static void imu_func(SurviveObject *so, int mask, const FLT *accelgyromag, uint32_t timecode, int id) {
	survive_default_imu_process(so, mask, accelgyromag, timecode, id);
}


void init_vr() {
	survive_ctx = survive_simple_init_with_logger(0, nullptr, &log_fn);
	if (survive_ctx) {
		auto ctx = survive_simple_get_ctx(survive_ctx);
		survive_install_imu_fn(ctx, imu_func);
		survive_simple_start_thread(survive_ctx);

		/*for (const SurviveSimpleObject *it = survive_simple_get_first_object(survive_ctx); it != 0; it = survive_simple_get_next_object(survive_ctx, it)) {
			printf("Found '%s'\n", survive_simple_object_name(it));
		}*/

		vr_active = true;
	}
}

bool VRDevice::button(int index) const {
	return (button_mask & (1 << index)) != 0;
}

bool VRDevice::clicked(int index) const {
	return ((button_mask & (1 << index)) != 0) and !(button_mask_prev & (1 << index));
}


float VRDevice::axis(int index) const {
	auto o = reinterpret_cast<SurviveSimpleObject*>(object);
	return (float)survive_simple_object_get_input_axis(o, (SurviveAxis)index);
}


void iterate_vr() {
	if (!survive_ctx)
		return;


	for (auto& d: vr_devices) {
		SurvivePose pose;
		auto o = reinterpret_cast<SurviveSimpleObject*>(d.object);
		survive_simple_object_get_latest_pose(o, &pose);
		d.pos = {(float)pose.Pos[0], (float)pose.Pos[2], (float)pose.Pos[1]};
		quaternion q;
		q.x = -(float)pose.Rot[1];
		q.y = -(float)pose.Rot[2];
		q.z = (float)pose.Rot[3];
		q.w = (float)pose.Rot[0];
		d.ang = quaternion::rotation_a(vec3::EX, pi/2) * q;
		//msg_write(format("%s   %s    %s", d.name, str(d.pos), str(d.ang)));

		d.button_mask_prev = d.button_mask;
		d.button_mask = survive_simple_object_get_button_mask(o);
	}


	SurviveSimpleEvent event = {SurviveSimpleEventType_None};
	//survive_simple_next_event(svctx, &event);
	survive_simple_wait_for_event(survive_ctx, &event);
	switch (event.event_type) {
	/*case SurviveSimpleEventType_PoseUpdateEvent: {
		const struct SurviveSimplePoseUpdatedEvent *pose_event = survive_simple_get_pose_updated_event(&event);
		SurvivePose pose = pose_event->pose;
		FLT timecode = pose_event->time;
		printf("%s %s (%7.3f): %f %f %f %f %f %f %f\n", survive_simple_object_name(pose_event->object),
			   survive_simple_serial_number(pose_event->object), timecode, pose.Pos[0], pose.Pos[1], pose.Pos[2],
			   pose.Rot[0], pose.Rot[1], pose.Rot[2], pose.Rot[3]);
		break;
	}
	case SurviveSimpleEventType_ButtonEvent: {
		const struct SurviveSimpleButtonEvent *button_event = survive_simple_get_button_event(&event);
		SurviveObjectSubtype subtype = survive_simple_object_get_subtype(button_event->object);
		printf("%s input %s (%d) ", survive_simple_object_name(button_event->object),
			   SurviveInputEventStr(button_event->event_type), button_event->event_type);

		FLT v1 = survive_simple_object_get_input_axis(button_event->object, SURVIVE_AXIS_TRACKPAD_X) / 2. + .5;

		if (button_event->button_id != 255) {
			printf(" button %16s (%2d) ", SurviveButtonsStr(subtype, button_event->button_id),
				   button_event->button_id);

			if (button_event->button_id == SURVIVE_BUTTON_SYSTEM) {
				FLT v = 1 - survive_simple_object_get_input_axis(button_event->object, SURVIVE_AXIS_TRIGGER);
				survive_simple_object_haptic(button_event->object, 30, v, .5);
			}
		}
		for (int i = 0; i < button_event->axis_count; i++) {
			printf(" %20s (%2d) %+5.4f   ", SurviveAxisStr(subtype, button_event->axis_ids[i]),
				   button_event->axis_ids[i], button_event->axis_val[i]);
		}
		printf("\n");
		break;
	}*/
	case SurviveSimpleEventType_ConfigEvent: {
		// we don't have correct type/subtype before this event!
		const auto cfg_event = survive_simple_get_config_event(&event);
		auto o = cfg_event->object;
		VRDeviceRole role = VRDeviceRole::None;
		auto type = survive_simple_object_get_type(o);
		auto subtype = survive_simple_object_get_subtype(o);
		if (type == SurviveSimpleObject_OBJECT) {
			if (subtype == SURVIVE_OBJECT_SUBTYPE_KNUCKLES_R)
				role = VRDeviceRole::ControllerRight;
			else if (subtype == SURVIVE_OBJECT_SUBTYPE_KNUCKLES_L)
				role = VRDeviceRole::ControllerLeft;
		} else if (type == SurviveSimpleObject_HMD) {
			role = VRDeviceRole::Headset;
		} else if (type == SurviveSimpleObject_LIGHTHOUSE) {
			role = VRDeviceRole::Lighthouse0;
		}
		for (auto& d: vr_devices)
			if (d.object == o) {
				d.role = role;
				d.name = survive_simple_object_name(o);
			}
		break;
	}
	case SurviveSimpleEventType_DeviceAdded: {
		const struct SurviveSimpleObjectEvent *obj_event = survive_simple_get_object_event(&event);
		auto o = obj_event->object;
		msg_write(format("VR Device Found: '%s'\n", survive_simple_object_name(obj_event->object)));
		vr_devices.add({(void*)o, VRDeviceRole::None, survive_simple_object_name(o)});
		break;
	}
	case SurviveSimpleEventType_None:
	default:
		break;
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
