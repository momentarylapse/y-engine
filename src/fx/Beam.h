/*
 * Beam.h
 *
 *  Created on: Aug 8, 2020
 *      Author: michi
 */

#ifndef SRC_FX_BEAM_H_
#define SRC_FX_BEAM_H_

#include "Particle.h"

class Beam : public Particle{
public:
	Beam(const vector &pos, const vector &length, float r, nix::Texture *tex, float ttl);

	void __init_beam__(const vector &pos, const vector &length, float r, nix::Texture *tex, float ttl);

	vector length;
};

#endif /* SRC_FX_BEAM_H_ */
