/*
 * Beam.cpp
 *
 *  Created on: Aug 8, 2020
 *      Author: michi
 */

#include "Beam.h"

Beam::Beam(const vector &_pos, const vector &_length, float _r, Texture *_tex, float _ttl) : Particle(_pos, _r, _tex, _ttl) {
	type = Type::BEAM;
	length = _length;
}

void Beam::__init_beam__(const vector &pos, const vector &length, float r, Texture *tex, float ttl) {
	new(this) Beam(pos, length, r, tex, ttl);
}

