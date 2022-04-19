/*
 * BaseClass.cpp
 *
 *  Created on: 16.08.2020
 *      Author: michi
 */

#include "BaseClass.h"

#include "Component.h"
#include "ComponentManager.h"
#include "../lib/kaba/syntax/Class.h"


#include "../lib/file/msg.h"


Array<BaseClass*> EntityManager::selection;



BaseClass::BaseClass(Type t) {
	type = t;
}

// hmm, no, let's not do too much here...
//   one might expect to call on_delete() here, but that's not possible,
//   since all outer destructors have been called at this point already
BaseClass::~BaseClass() {
	//msg_write("~Entity " + i2s((int)type));
	for (auto *c: components)
		ComponentManager::delete_component(c);
/*	//msg_write("~Entity " + i2s((int)type));
	if (EntityManager::enabled) {
		//msg_write("auto unreg...");
		world.unregister(this);
	}
	msg_write("/~Entity " + i2s((int)type));*/
}

void BaseClass::on_init_rec() {
	//msg_write("init rec");
	on_init();
	for (auto c: components) {
		//msg_write(" -> " + c->component_type->name);
		c->on_init();
	}
}

void BaseClass::on_delete_rec() {
	for (auto c: components)
		c->on_delete();
	on_delete();
}


// TODO (later) optimize...
Component *BaseClass::add_component(const kaba::Class *type, const string &var) {
	auto c = add_component_no_init(type, var);

//	c->on_init();
	// don't init now, wait until the on_init_rec() later (via World.register_entity())!
	return c;
}

Component *BaseClass::add_component_no_init(const kaba::Class *type, const string &var) {
	auto c = ComponentManager::create_component(type, var);
	components.add(c);
	c->owner = this;
	return c;
}

void BaseClass::_add_component_external_(Component *c) {
	ComponentManager::add_to_list(c, ComponentManager::get_component_type_family(c->component_type));
	components.add(c);
	c->owner = this;
	//c->on_init();
}

Component *BaseClass::_get_component_untyped_(const kaba::Class *type) const {
	//msg_write("get " + type->name);
	for (auto *c: components) {
		//msg_write(p2s(c->component_type));
		//msg_write("... " + c->component_type->name);
		if (c->component_type->is_derived_from(type))
			return c;
	}
	return nullptr;
}



void EntityManager::reset() {
	selection.clear();
}


void EntityManager::delete_later(BaseClass *p) {
	selection.add(p);
}

void EntityManager::delete_selection() {
	for (auto *p: selection)
		delete p;
	selection.clear();
}

