/*
 * ComponentManager.cpp
 *
 *  Created on: Jul 13, 2021
 *      Author: michi
 */

#include "ComponentManager.h"
#include "Component.h"
#include "../lib/base/map.h"
#include "../lib/config.h"
#ifdef _X_ALLOW_X_
#include "../meta.h"
#include "../plugins/PluginManager.h"
#include "../lib/kaba/syntax/Class.h"
#include "../lib/kaba/syntax/Function.h"
#endif


class ComponentListX {
public:
	ComponentManager::List list;
	bool needs_update = false;
	const kaba::Class *type_family = nullptr;

	void add(Component *c) {
		list.add(c);
	}
	void remove(Component *c) {
		foreachi (auto *cc, list, i)
			if (cc == c) {
				list.erase(i);
				break;
			}
	}
};

Map<const kaba::Class*, ComponentListX> component_lists;

bool class_func_did_override(const kaba::Class *type, const string &fname) {
	for (auto f: weak(type->functions))
		if (f->name == fname)
			return f->name_space != type->get_root();
	return false;
}

ComponentListX *get_list_x(const kaba::Class *type_family) {
	if (component_lists.find(type_family) >= 0) {
		return &component_lists[type_family];
	} else {
		ComponentListX list;
		list.type_family = type_family;
		list.needs_update = class_func_did_override(type_family, "on_iterate");
		component_lists.set(type_family, list);
		return &component_lists[type_family];
	}
}


void ComponentManager::add_to_list(Component *c, const kaba::Class *type_family) {
	auto l = get_list_x(type_family);
	l->add(c);
}


const kaba::Class *ComponentManager::get_component_type_family(const kaba::Class *type) {
	while (type->parent) {
		if (type->parent->name == "Component")
			return type;
		type = type->parent;
	}
	return type;
}

// TODO (later) optimize...
Component *ComponentManager::create_component(const kaba::Class *type, const string &var) {
#ifdef _X_ALLOW_X_
	//Component *c = nullptr;
	auto c = (Component*)plugin_manager.create_instance(type, var);
	c->type = type;
	auto type_family = get_component_type_family(type);
	add_to_list(c, type_family);
	return c;
#else
	return nullptr;
#endif
}

void ComponentManager::delete_component(Component *c) {
	auto type_family = get_component_type_family(c->type);
	auto list = get_list_x(type_family);
	list->remove(c);
	delete c;
}


ComponentManager::List *ComponentManager::get_list(const kaba::Class *type_family) {
	return &get_list_x(type_family)->list;
}

void ComponentManager::iterate(float dt) {
	for (auto &l: component_lists)
		if (l.value.needs_update)
			for (auto *c: l.value.list)
				c->on_iterate(dt);
}
