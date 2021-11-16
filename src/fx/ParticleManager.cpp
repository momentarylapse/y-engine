/*
 * ParticleManager.cpp
 *
 *  Created on: Aug 9, 2020
 *      Author: michi
 */


#include "ParticleManager.h"
#include "Particle.h"
#include "Beam.h"
#include "../lib/file/msg.h"


ParticleGroup::ParticleGroup(Texture *t) {
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
	if (p->type == Particle::Type::PARTICLE)
		particles.add(p);
	else if (p->type == Particle::Type::BEAM)
		beams.add((Beam*)p);
}

bool ParticleGroup::unregister(Particle *p) {
	if (p->type == Entity::Type::PARTICLE) {
		foreachi (auto *pp, particles, i)
			if (pp == p) {
				//msg_write("  -> PARTICLE");
				particles.erase(i);
				return true;
			}
	} else if (p->type == Entity::Type::BEAM) {
		foreachi (auto *pp, beams, i)
			if (pp == p) {
				//msg_write("  -> BEAM");
				beams.erase(i);
				return true;
			}
	}
	return false;
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

bool ParticleManager::unregister(Particle *p) {
	for (auto *g: groups)
		if (g->unregister(p))
			return true;
	return false;
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
				//msg_write("PARTICLE SUICIDE");
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

void ParticleManager::shift_all(const vector &dpos) {
	for (auto g: groups) {
		for (auto *p: g->particles)
			p->pos += dpos;
		for (auto *p: g->beams)
			p->pos += dpos;
	}
}


