/*
 * ParticleManager.cpp
 *
 *  Created on: Aug 9, 2020
 *      Author: michi
 */


#include "ParticleManager.h"
#include "Particle.h"
#include "Beam.h"


ParticleGroup::ParticleGroup(nix::Texture *t) {
	texture = t;
	ubo = nullptr;
#if USE_API_VULKAN
	ubo = new UniformBuffer(256);
	dset = rp_create_dset_fx(t, ubo);
#endif
}

ParticleGroup::~ParticleGroup() {
	for (auto *p: particles)
		delete p;
	for (auto *b: beams)
		delete b;
#if USE_API_VULKAN
	delete dset;
	delete ubo;
#endif
}

void ParticleGroup::add(Particle *p) {
	if (p->type == ParticleType::PARTICLE)
		particles.add(p);
	else if (p->type == ParticleType::BEAM)
		beams.add((Beam*)p);
}



void ParticleManager::add(Particle *p) {
	for (auto *g: groups)
		if (g->texture == p->texture) {
			g->add(p);
			return;
		}
	auto *gg = new ParticleGroup(p->texture);
	gg->add(p);
	groups.add(gg);
}

void ParticleManager::clear() {
	for (auto *g: groups)
		delete g;
	groups.clear();
}

static void iterate_particles(Array<Particle*> *particles, float dt) {
	foreachi (auto p, *particles, i) {
		p->pos += p->vel * dt;
		if (p->suicidal) {
			p->time_to_live -= dt;
			if (p->time_to_live < 0) {
				particles->erase(i);
				delete p;
				i --;
				continue;
			}
		}
		p->on_iterate(dt);
	}
}

void ParticleManager::iterate(float dt) {
	for (auto g: groups) {
		iterate_particles(&g->particles, dt);
		iterate_particles((Array<Particle*>*)&g->beams, dt);
	}
}

