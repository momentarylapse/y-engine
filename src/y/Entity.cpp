/*
 * Entity.cpp
 *
 *  Created on: Jul 15, 2021
 *      Author: michi
 */

#include "Entity.h"
#include "../lib/math/matrix.h"


Entity::Entity() : Entity(vector::ZERO, quaternion::ID) {}

Entity::Entity(const vector &_pos, const quaternion &_ang) : BaseClass(BaseClass::Type::ENTITY) {
	pos = _pos;
	ang = _ang;
	parent = nullptr;
	object_id = -1;
}

matrix Entity::get_local_matrix() const {
	return matrix::translation(pos) * matrix::rotation(ang);
}

matrix Entity::get_matrix() const {
	if (parent)
		return parent->get_matrix() * get_local_matrix();
	return get_local_matrix();
}


Entity *Entity::root() const {
	Entity *next = const_cast<Entity*>(this);
	while (next->parent)
		next = next->parent;
	return next;
}

