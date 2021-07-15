/*
 * Entity3D.cpp
 *
 *  Created on: Jul 15, 2021
 *      Author: michi
 */

#include "Entity3D.h"
#include "../lib/math/matrix.h"


Entity3D::Entity3D(Type type) : Entity(type) {
	pos = v_0;
	ang = quaternion::ID;
}

matrix Entity3D::get_matrix() const {
	return matrix::translation(pos) * matrix::rotation(ang);
}


