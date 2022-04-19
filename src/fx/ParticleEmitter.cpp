/*
 * ParticleEmitter.cpp
 *
 *  Created on: 19 Apr 2022
 *      Author: michi
 */

#include "ParticleEmitter.h"
#include "Particle.h"
#include "../lib/math/math.h"
#include "../lib/math/random.h"

extern Texture *tex_white;

static Random pe_random;

ParticleEmitter::ParticleEmitter() : BaseClass(BaseClass::Type::PARTICLE_EMITTER) {
	time_to_live = 1;
	spawn_dt = 0.1f;
	texture = tex_white;

	pos = vector::ZERO;
	spawn_vel = vector(0,0,100);
	spawn_dvel = 20;
	spawn_radius = 10;
	spawn_dradius = 5;
}

ParticleEmitter::~ParticleEmitter() {
}

void ParticleEmitter::on_iterate_particle(Particle *p, float dt) {
	p->pos += p->vel * dt;
}

void ParticleEmitter::on_iterate(float dt) {
	/*int N = time_to_live * spawn_dt;

	phase += dt;
	while (phase > spawn_dt) {
		auto p = &particles[next_index];
		if (particles.num < N) {
			particles.resize(next_index + 1);
			p = &particles[next_index];
		}

		// new
		p->texture = texture;
		p->pos = pos;
		p->vel = spawn_vel + pe_random.in_ball(spawn_dvel);
		p->radius = spawn_radius + randf(spawn_dradius);
		p->col = White;
		on_create_particle(p);

		phase -= spawn_dt;
		next_index ++;
		if (next_index >= N)
			next_index = 0;
	}

	for (auto &p: particles)
		on_iterate_particle(&p, dt);*/
}

