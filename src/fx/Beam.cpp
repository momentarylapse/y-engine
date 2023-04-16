/*
 * Beam.cpp
 *
 *  Created on: Aug 8, 2020
 *      Author: michi
 */

#include "Beam.h"
#include "../graphics-impl.h"

Beam::Beam(const vec3 &_pos, const vec3 &_length, const color& _col, float _r, float _ttl) : Particle(_pos, _col, _r, _ttl) {
	length = _length;
}


LegacyBeam::LegacyBeam(const vec3 &_pos, const vec3 &_length, float _r, shared<Texture> _tex, float _ttl) : LegacyParticle(_pos, _r, _tex, _ttl) {
	type = Type::LEGACY_BEAM;
	length = _length;
}

void LegacyBeam::__init_beam__(const vec3 &pos, const vec3 &length, float r, shared<Texture> tex, float ttl) {
	new(this) LegacyBeam(pos, length, r, tex, ttl);
}

