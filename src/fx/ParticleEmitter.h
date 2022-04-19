/*
 * ParticleEmitter.h
 *
 *  Created on: 19 Apr 2022
 *      Author: michi
 */

#ifndef SRC_FX_PARTICLEEMITTER_H_
#define SRC_FX_PARTICLEEMITTER_H_

#include "../graphics-fwd.h"
#include "../lib/base/base.h"
#include "../lib/math/vector.h"
#include "../lib/image/color.h"
#include "../y/BaseClass.h"

class Particle;

class ParticleEmitter : public BaseClass {
public:
	ParticleEmitter();
	virtual ~ParticleEmitter();

	virtual void on_create_particle(Particle *p) {}
	virtual void on_iterate_particle(Particle *p, float dt);
	void on_iterate(float dt) override;

	float time_to_live;
	float phase = 0;
	float spawn_dt;
	int next_index = 0;
	Texture *texture;
	vector pos;
	vector spawn_vel;
	float spawn_dvel;
	float spawn_radius;
	float spawn_dradius;

	Array<Particle> particles;
};

#endif /* SRC_FX_PARTICLEEMITTER_H_ */
