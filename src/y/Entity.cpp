/*
 * Entity.cpp
 *
 *  Created on: 16.08.2020
 *      Author: michi
 */

#include "Entity.h"
#include "Component.h"


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


// TODO (later) optimize...
void Entity::add_component(Component *c, kaba::Class *type) {
	components.add(c);
	c->type = type;
	c->owner = this;
}

Component *Entity::get_component(kaba::Class *type) const {
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

