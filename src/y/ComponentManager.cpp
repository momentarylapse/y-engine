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
	const kaba::Class *type = nullptr;

	void add(Component *c) {
		list.add(c);
	}
};

Map<const kaba::Class*, ComponentListX> component_lists;

bool class_func_did_override(const kaba::Class *type, const string &fname) {
	for (auto f: weak(type->functions))
		if (f->name == fname)
			return f->name_space != type->get_root();
	return false;
}

ComponentListX *get_list_x(const kaba::Class *type) {
	if (component_lists.find(type) >= 0) {
		return &component_lists[type];
	} else {
		ComponentListX list;
		list.type = type;
		list.needs_update = class_func_did_override(type, "on_iterate");
		component_lists.set(type, list);
		return &component_lists[type];
	}
}


void ComponentManager::add_to_list(Component *c, const kaba::Class *type) {
	auto l = get_list_x(type);
	l->add(c);
}


// TODO (later) optimize...
Component *ComponentManager::create_component(const kaba::Class *type) {
#ifdef _X_ALLOW_X_
	auto c = (Component*)plugin_manager.create_instance(type, {});
	c->type = type;
	add_to_list(c, type);
	return c;
#else
	return nullptr;
#endif
}


ComponentManager::List *ComponentManager::get_list(const kaba::Class *type) {
	return &get_list_x(type)->list;
}

void ComponentManager::iterate(float dt) {
	for (auto &l: component_lists)
		if (l.value.needs_update)
			for (auto *c: l.value.list)
				c->on_iterate(dt);
}

