/*
 * Controller.cpp
 *
 *  Created on: 02.01.2020
 *      Author: michi
 */

#include "Controller.h"

Controller::Controller() : Entity(Type::CONTROLLER) {}

void Controller::__init__() {
	new(this) Controller;
}

void Controller::__delete__() {
	this->Controller::~Controller();
}

