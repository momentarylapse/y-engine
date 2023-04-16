/*
 * ParticleEmitter.cpp
 *
 *  Created on: 19 Apr 2022
 *      Author: michi
 */

#include "ParticleEmitter.h"
#include "Particle.h"
#include "../graphics-impl.h"
#include "../y/Entity.h"
#include "../lib/math/math.h"
#include "../lib/math/random.h"
#include "../lib/os/msg.h"

extern Texture *tex_white;
static Random pe_random;

const kaba::Class *ParticleGroup::_class = nullptr;
const kaba::Class *ParticleEmitter::_class = nullptr;

ParticleGroup::ParticleGroup() {
	texture = tex_white;
	//pos = vec3::ZERO;
}


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
		auto p = emit_particle(owner->pos, White, spawn_radius);
		p->time_to_live = spawn_time_to_live;
		p->suicidal = false;//(p->time_to_live > 0);
		on_init_particle(p);
		p->pos += p->vel * tt;
	}

	ParticleGroup::on_iterate(dt);
}

