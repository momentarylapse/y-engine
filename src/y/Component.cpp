/*
 * Component.cpp
 *
 *  Created on: Jul 12, 2021
 *      Author: michi
 */

#include "Component.h"
#include "../lib/base/base.h"

Component::Component() {
	owner = nullptr;
	type = nullptr;
}

Component::~Component() {}

void Component::__init__() {
	new(this) Component();
}

void Component::__delete__() {
	this->Component::~Component();
}

