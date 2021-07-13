/*
 * Component.h
 *
 *  Created on: Jul 12, 2021
 *      Author: michi
 */

#pragma once

class Entity;
namespace kaba {
	class Class;
}

class Component {
public:
	Component();

	void __init__();

	Entity *owner;
	const kaba::Class *type;
};
