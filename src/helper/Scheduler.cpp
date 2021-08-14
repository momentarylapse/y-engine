/*
 * Scheduler.cpp
 *
 *  Created on: Jul 24, 2021
 *      Author: michi
 */

#include "Scheduler.h"
#include "../plugins/PluginManager.h"
#include "../plugins/Controller.h"
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

void Scheduler::iterate_subscriptions(float dt) {
	for (auto &l: schedule_listener) {
		l.t -= dt;
		if (l.t <= 0) {
			l.f();
			l.t = l.dt;
		}
	}
}

void Scheduler::handle_iterate(float dt) {
	for (auto *c: PluginManager::controllers)
		c->on_iterate(dt);

	iterate_subscriptions(dt);
}

void Scheduler::handle_iterate_pre(float dt) {
	for (auto *c: PluginManager::controllers)
		c->on_iterate_pre(dt);
}

void Scheduler::handle_input() {
	for (auto *c: PluginManager::controllers)
		c->on_input();
}

void Scheduler::handle_draw_pre() {
	for (auto *c: PluginManager::controllers)
		c->on_draw_pre();
}

void Scheduler::handle_render_inject() {
	for (auto *c: PluginManager::controllers)
		c->on_render_inject();
}

void Scheduler::handle_render_inject2() {
	for (auto *c: PluginManager::controllers)
		c->on_render_inject2();
}
