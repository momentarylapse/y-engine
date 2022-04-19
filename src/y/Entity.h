/*
 * Entity.h
 *
 *  Created on: Jul 15, 2021
 *      Author: michi
 */

#pragma once

#include "../lib/math/vector.h"
#include "../lib/math/quaternion.h"
#include "BaseClass.h"

class matrix;


class Entity : public BaseClass {
public:
	Entity();
	Entity(const vector &pos, const quaternion &ang);

	vector pos;
	quaternion ang;
	matrix get_local_matrix() const;
	matrix get_matrix() const;

	int object_id;
	Entity *parent;
	Entity *_cdecl root() const;
};

