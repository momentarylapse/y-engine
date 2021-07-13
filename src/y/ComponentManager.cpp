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
#endif


Map<const kaba::Class*, ComponentManager::List> component_lists;


void ComponentManager::add_to_list(Component *c, const kaba::Class *type) {
	auto l = get_list(type);
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
	if (component_lists.find(type) >= 0) {
		return &component_lists[type];
	} else {
		component_lists.set(type, {});
		return &component_lists[type];
	}
}

