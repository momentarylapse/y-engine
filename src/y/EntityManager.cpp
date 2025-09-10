//
// Created by michi on 9/10/25.
//

#include "EntityManager.h"
#include "Entity.h"

EntityManager::EntityManager() = default;

EntityManager::~EntityManager() {
	reset();
}

Entity *EntityManager::create_entity(const vec3 &pos, const quaternion &ang) {
	auto e = new Entity(pos, ang);
	entities.add(e);
	return e;
}

void EntityManager::delete_entity(Entity* e) {
	int index = entities.find(e);
	if (index < 0)
		return;

	//e->on_delete_rec();

	entities.erase(index);
	delete e;
}

void EntityManager::shift_all(const vec3 &dpos) {
	for (auto *e: entities)
		e->pos += dpos;
}


void EntityManager::reset() {
	for (auto *o: entities)
		delete o;
	entities.clear();
}



