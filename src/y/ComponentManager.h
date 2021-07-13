/*
 * ComponentManager.h
 *
 *  Created on: Jul 13, 2021
 *      Author: michi
 */

#pragma once

#include "../lib/base/base.h"

class Component;
namespace kaba {
	class Class;
}


class ComponentManager {
public:
	using List = Array<Component*>;

	static Component *create_component(const kaba::Class *type, const string &var);
	static void delete_component(Component *c);
	static List *get_list(const kaba::Class *type_family);

	static void add_to_list(Component *c, const kaba::Class *type_family);

	static void iterate(float dt);
};

