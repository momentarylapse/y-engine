/*
 * Entity.cpp
 *
 *  Created on: 16.08.2020
 *      Author: michi
 */

#include "Entity.h"
#include "Component.h"
#include "ComponentManager.h"


bool EntityManager::enabled = true;
Array<Entity*> EntityManager::selection;



Entity::Entity(Type t) {
	type = t;
}

// hmm, no, let's not do too much here...
//   one might expect to call on_delete() here, but that's not possible,
//   since all outer destructors have been called at this point already
Entity::~Entity() {
/*	//msg_write("~Entity " + i2s((int)type));
	if (EntityManager::enabled) {
		//msg_write("auto unreg...");
		world.unregister(this);
	}
	//msg_write("/~Entity " + i2s((int)type));*/
}

void Entity::on_init_rec() {
	on_init();
	for (auto c: components)
		c->on_init();
}


// TODO (later) optimize...
Component *Entity::add_component(const kaba::Class *type) {
	auto c = ComponentManager::create_component(type);
	components.add(c);
	c->owner = this;
	c->on_init();
	return c;
}

Component *Entity::get_component(const kaba::Class *type) const {
	for (auto *c: components)
		if (c->type == type)
			return c;
	return nullptr;
}



void EntityManager::reset() {
	selection.clear();
}


void EntityManager::delete_later(Entity *p) {
	selection.add(p);
}

void EntityManager::delete_selection() {
	for (auto *p: selection)
		delete p;
	selection.clear();
}

