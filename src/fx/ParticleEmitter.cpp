/*
 * ParticleEmitter.cpp
 *
 *  Created on: 19 Apr 2022
 *      Author: michi
 */

#include "ParticleEmitter.h"
#include "Particle.h"
#include "../graphics-impl.h"
#include "../lib/math/math.h"
#include "../lib/math/random.h"

#if !NEW_PARTICLES
#include "../world/World.h"
#include "../lib/os/msg.h"
#endif

extern Texture *tex_white;

static Random pe_random;


ParticleGroup::ParticleGroup() : BaseClass(BaseClass::Type::PARTICLE_GROUP) {
	texture = tex_white;
	pos = vec3::ZERO;
}


#if NEW_PARTICLES
Particle* ParticleGroup::emit_particle(const vec3& pos, const color& col, float r) {
	particles.add(Particle(pos, col, r, -1));
	return &particles.back();
}

void ParticleGroup::on_iterate(float dt) {
	for (auto &p: particles) {
		on_iterate_particle(&p, dt);
		p.pos += p.vel * dt;
	}
	for (int i=0; i<particles.num; i++) {
		auto& p = particles[i];
		//p->pos += p->vel * dt;
		p.time_to_live -= dt;
		if (p.time_to_live < 0 /*and p->suicidal*/) {
			particles.erase(i);
			i --;
		}
	}
}
#else
LegacyParticle* ParticleGroup::emit_particle(const vec3& pos, const color& col, float r) {
	auto p = world.add_legacy_particle(new LegacyParticle(pos, r, texture, -1));
	p->col = col;
	particles.add(p);
	return p;
}

void ParticleGroup::on_iterate(float dt) {
	for (auto p: particles) {
		on_iterate_particle(p, dt);
	}
	for (int i=0; i<particles.num; i++) {
		auto p = particles[i];
		//p->pos += p->vel * dt;
		p->time_to_live -= dt;
		if (p->time_to_live < 0 /*and p->suicidal*/) {
			particles.erase(i);
			world.delete_legacy_particle(p);
			i --;
		}
	}
}
#endif


ParticleEmitter::ParticleEmitter() {
	spawn_time_to_live = 1;
	spawn_dt = 0.1f;

	spawn_vel = vec3(0,0,100);
	spawn_dvel = 20;
	spawn_radius = 10;
	spawn_dradius = 5;
}

void ParticleEmitter::__init__() {
	new(this) ParticleEmitter;
}

void ParticleEmitter::on_iterate(float dt) {
	tt += dt;
	while (tt >= spawn_dt) {
		tt -= spawn_dt;
		auto p = emit_particle(pos, White, spawn_radius);
		p->time_to_live = spawn_time_to_live;
		p->suicidal = false;//(p->time_to_live > 0);
		on_init_particle(p);
		p->pos += p->vel * tt;
	}

	ParticleGroup::on_iterate(dt);
}

