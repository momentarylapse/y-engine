/*
 * Entity.cpp
 *
 *  Created on: 16.08.2020
 *      Author: michi
 */

#include "Entity.h"


bool EntityManager::enabled = true;
Array<Entity*> EntityManager::selection;



Entity::Entity(Type t) {
	type = t;
}

Entity::~Entity() {
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

