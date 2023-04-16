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
#include "../lib/base/pointer.h"
#include "../lib/math/vec3.h"
#include "../lib/image/color.h"
#include "../y/BaseClass.h"
#include "../y/Component.h"
#include "Particle.h"
#include "Beam.h"

#define NEW_PARTICLES 0

class ParticleGroup : public BaseClass /*Component*/ {
public:
	ParticleGroup();

#if NEW_PARTICLES
	Particle* emit_particle(const vec3& pos, const color& col, float r);
	virtual void on_iterate_particle(Particle *p, float dt) {}
#else
	LegacyParticle* emit_particle(const vec3& pos, const color& col, float r);
	virtual void on_iterate_particle(LegacyParticle *p, float dt) {}
#endif
	void on_iterate(float dt) override;

	Texture* texture;
	vec3 pos;

#if NEW_PARTICLES
	Array<Particle> particles;
#else
	Array<LegacyParticle*> particles;
#endif
};

class BeamGroup : public Component {
public:
	BeamGroup();

	Particle& emit_beam(const vec3& pos, const vec3& length, const color& col, float r);
	virtual void on_iterate_beam(Beam *p, float dt) {}
	void on_iterate(float dt) override;

	shared<Texture> texture;
	vec3 pos;

	Array<Beam> beams;
};

class ParticleEmitter : public ParticleGroup {
public:
	ParticleEmitter();
	void __init__();

#if NEW_PARTICLES
	virtual void on_init_particle(Particle *p) {}
#else
	virtual void on_init_particle(LegacyParticle *p) {}
#endif
	void on_iterate(float dt) override;

	float spawn_time_to_live;
	float tt = 0;
	float spawn_dt;
	//int next_index = 0;
	vec3 spawn_vel;
	float spawn_dvel;
	float spawn_radius;
	float spawn_dradius;
};

#endif /* SRC_FX_PARTICLEEMITTER_H_ */
