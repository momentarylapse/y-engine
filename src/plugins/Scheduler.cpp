/*
 * Scheduler.cpp
 *
 *  Created on: Jul 24, 2021
 *      Author: michi
 */

#include "Scheduler.h"
#include "../lib/kaba/kaba.h"

typedef void schedule_callback();

struct ScheduleListener {
	float dt;
	float t;
	schedule_callback *f;
};
static Array<ScheduleListener> schedule_listener;

void Scheduler::reset() {
	schedule_listener.clear();
}

void Scheduler::subscribe(float dt, kaba::Function *f) {
	schedule_listener.add({dt, dt, (schedule_callback*)(void*)(int_p)f->address});
}

void Scheduler::iterate(float dt) {
	for (auto &l: schedule_listener) {
		l.t -= dt;
		if (l.t <= 0) {
			l.f();
			l.t = l.dt;
		}
	}
}
