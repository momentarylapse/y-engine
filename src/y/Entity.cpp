/*
 * Entity.cpp
 *
 *  Created on: 16.08.2020
 *      Author: michi
 */

#include "Entity.h"
#include "../world/World.h"
#include "../lib/file/msg.h"


bool EntityManager::enabled = true;
Array<Entity*> EntityManager::selection;



Entity::Entity(Type t) {
	type = t;
}

Entity::~Entity() {
	//msg_write("~Entity " + i2s((int)type));
	if (EntityManager::enabled) {
		//msg_write("auto unreg...");
		world.unregister(this);
	}
	//msg_write("/~Entity " + i2s((int)type));
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

