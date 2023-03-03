/*
 * Beam.cpp
 *
 *  Created on: Aug 8, 2020
 *      Author: michi
 */

#include "Beam.h"
#include "../graphics-impl.h"


Beam::Beam(const vec3 &_pos, const vec3 &_length, float _r, shared<Texture> _tex, float _ttl) : Particle(_pos, _r, _tex, _ttl) {
	type = Type::BEAM;
	length = _length;
}

void Beam::__init_beam__(const vec3 &pos, const vec3 &length, float r, shared<Texture> tex, float ttl) {
	new(this) Beam(pos, length, r, tex, ttl);
}

