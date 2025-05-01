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

void iterate_vr() {
	if (!survive_ctx)
		return;


	for (auto& d: vr_devices) {
		SurvivePose pose;
		auto o = reinterpret_cast<SurviveSimpleObject*>(d.object);
		survive_simple_object_get_latest_pose(o, &pose);
		d.pos = {(float)pose.Pos[0], (float)pose.Pos[1], (float)pose.Pos[2]};
		d.ang.x = (float)pose.Rot[0];
		d.ang.y = (float)pose.Rot[1];
		d.ang.z = (float)pose.Rot[2];
		d.ang.w = (float)pose.Rot[3];
		msg_write(format("%s   %s    %s", d.name, str(d.pos), str(d.ang)));
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
		const struct SurviveSimpleConfigEvent *cfg_event = survive_simple_get_config_event(&event);
		/*printf("(%f) %s received configuration of length %u type %d-%d\n", cfg_event->time,
			   survive_simple_object_name(cfg_event->object), (unsigned)strlen(cfg_event->cfg),
			   survive_simple_object_get_type(cfg_event->object),
			   survive_simple_object_get_subtype(cfg_event->object));*/
		break;
	}
	case SurviveSimpleEventType_DeviceAdded: {
		const struct SurviveSimpleObjectEvent *obj_event = survive_simple_get_object_event(&event);
		//printf("(%f) Found '%s'\n", obj_event->time, survive_simple_object_name(obj_event->object));
		auto o = obj_event->object;
		msg_error(format("(%f) Found '%s'\n", obj_event->time, survive_simple_object_name(obj_event->object)));
		vr_devices.add({(void*)o, (int)survive_simple_object_get_type(o), survive_simple_object_name(o)});
		break;
	}
	case SurviveSimpleEventType_None:
	default:
		break;
	}
}

VRDevice* get_vr_device(int index) {
	for (auto& d: vr_devices) {
		// TODO come up with a better mapping...
		if (index == 0 and d.name == "KN0")
			return &d;
		if (index == 1 and d.name == "KN1")
			return &d;
		if (index == 2 and d.type == SurviveSimpleObject_HMD)
			return &d;
		if (index == 3 and d.type == SurviveSimpleObject_LIGHTHOUSE)
			return &d;
	}
	return nullptr;
}

#else

void init_vr() {}

void iterate_vr() {}

VRDevice* get_vr_device(int index) {
	return nullptr;
}

#endif
}
