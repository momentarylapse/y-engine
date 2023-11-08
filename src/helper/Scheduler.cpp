/*
 * Scheduler.cpp
 *
 *  Created on: Jul 24, 2021
 *      Author: michi
 */

#include "Scheduler.h"
#include "PerformanceMonitor.h"
#include "../y/ComponentManager.h"
#include "../plugins/PluginManager.h"
#include "../plugins/Controller.h"
#include "../lib/kaba/kaba.h"

struct ScheduleListener {
	float dt;
	float t;
	const Callable<void()> *f;
};
static Array<ScheduleListener> schedule_listener;
static int ch_iterate = -1;
extern int ch_controller;
static int ch_con_iter_pre = -1;
static int ch_con_input = -1;
static int ch_con_draw_pre = -1;

void Scheduler::init(int ch_iter_parent) {
	ch_iterate = PerformanceMonitor::create_channel("scheduler", ch_iter_parent);
	ch_con_iter_pre = PerformanceMonitor::create_channel("it0", ch_iter_parent);
	ch_con_input = PerformanceMonitor::create_channel("in0", ch_iter_parent);
	ch_con_draw_pre = PerformanceMonitor::create_channel("dr0", ch_iter_parent);
}

void Scheduler::reset() {
	schedule_listener.clear();
}

void Scheduler::subscribe(float dt, const Callable<void()> &f) {
	schedule_listener.add({dt, dt, &f});
}

void Scheduler::iterate_subscriptions(float dt) {
	for (auto &l: schedule_listener) {
		l.t -= dt;
		if (l.t <= 0) {
			(*l.f)();
			l.t = l.dt;
		}
	}
}

void Scheduler::handle_iterate(float dt) {
	PerformanceMonitor::begin(ch_controller);
	for (auto *c: PluginManager::controllers) {
		PerformanceMonitor::begin(c->ch_iterate);
		c->on_iterate(dt);
		PerformanceMonitor::end(c->ch_iterate);
	}
	PerformanceMonitor::end(ch_controller);

	PerformanceMonitor::begin(ch_iterate);
	iterate_subscriptions(dt);
	PerformanceMonitor::end(ch_iterate);

	ComponentManager::iterate(dt);
}

void Scheduler::handle_iterate_pre(float dt) {
	PerformanceMonitor::begin(ch_con_iter_pre);
	for (auto *c: PluginManager::controllers)
		c->on_iterate_pre(dt);
	PerformanceMonitor::end(ch_con_iter_pre);
}

void Scheduler::handle_input() {
	PerformanceMonitor::begin(ch_con_input);
	for (auto *c: PluginManager::controllers)
		c->on_input();
	PerformanceMonitor::end(ch_con_input);
}

void Scheduler::handle_draw_pre() {
	PerformanceMonitor::begin(ch_con_draw_pre);
	for (auto *c: PluginManager::controllers)
		c->on_draw_pre();
	PerformanceMonitor::end(ch_con_draw_pre);
}

void Scheduler::handle_render_inject() {
	for (auto *c: PluginManager::controllers)
		c->on_render_inject();
}

void Scheduler::handle_render_inject2() {
	for (auto *c: PluginManager::controllers)
		c->on_render_inject2();
}
