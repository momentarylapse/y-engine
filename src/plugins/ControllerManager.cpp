//
// Created by Michael Ankele on 2024-10-13.
//

#include "ControllerManager.h"
#include "Controller.h"
#include "PluginManager.h"
#include "../helper/PerformanceMonitor.h"
#include "../helper/Scheduler.h"
#include "../lib/kaba/kaba.h"
#include "../lib/os/path.h"
#include "../lib/os/msg.h"


Array<Controller*> ControllerManager::controllers;
int ch_controller;

void ControllerManager::init(int ch_iter) {
	ch_controller = PerformanceMonitor::create_channel("controller", ch_iter);
}

void ControllerManager::reset() {
	msg_write("del controller");
	for (auto *c: controllers)
		delete c;
	controllers.clear();
	Scheduler::reset();
}

void ControllerManager::add_controller(const Path &name, const Array<TemplateDataScriptVariable> &variables) {
	msg_write("add controller: " + name.str());
	auto type = PluginManager::find_class_derived(name, "ui.Controller");;
	auto *c = (Controller*)PluginManager::create_instance(type, variables);
	c->_class = type;
	c->ch_iterate = PerformanceMonitor::create_channel(type->long_name(), ch_controller);

	controllers.add(c);
	c->on_init();
}

Controller *ControllerManager::get_controller(const kaba::Class *type) {
	for (auto c: controllers)
		if (c->_class == type)
			return c;
	return nullptr;
}


