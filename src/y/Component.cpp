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

void Component::__init__() {
	new(this) Component();
}

