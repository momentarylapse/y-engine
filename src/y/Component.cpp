/*
 * Component.cpp
 *
 *  Created on: Jul 12, 2021
 *      Author: michi
 */

#include "Component.h"
#include "../lib/base/base.h"
#include "../plugins/PluginManager.h"

Component::Component() {
	owner = nullptr;
	component_type = nullptr;
}

Component::~Component() {}

void Component::__init__() {
	new(this) Component();
}

void Component::__delete__() {
	this->Component::~Component();
}

void Component::set_variables(const string &var) {
	plugin_manager.assign_variables(this, component_type, var);
}

