/*
 * Beam.h
 *
 *  Created on: Aug 8, 2020
 *      Author: michi
 */

#pragma once

#include "Particle.h"

class Beam : public Particle{
public:
	Beam(const vector &pos, const vector &length, float r, Texture *tex, float ttl);

	void __init_beam__(const vector &pos, const vector &length, float r, Texture *tex, float ttl);

	vector length;
};
