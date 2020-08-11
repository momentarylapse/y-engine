/*
 * Beam.cpp
 *
 *  Created on: Aug 8, 2020
 *      Author: michi
 */

#include "Beam.h"

Beam::Beam(const vector &_pos, const vector &_length, float _r, nix::Texture *_tex, float _ttl) : Particle(_pos, _r, _tex, _ttl) {
	type = ParticleType::BEAM;
	length = _length;
}

void Beam::__init_beam__(const vector &pos, const vector &length, float r, nix::Texture *tex, float ttl) {
	new(this) Beam(pos, length, r, tex, ttl);
}

