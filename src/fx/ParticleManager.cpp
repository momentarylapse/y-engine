/*
 * ParticleManager.cpp
 *
 *  Created on: Aug 9, 2020
 *      Author: michi
 */


#include "ParticleManager.h"
#include "Particle.h"
#include "Beam.h"
#include "ParticleEmitter.h"
#include "../lib/os/msg.h"


LegacyParticleGroup::LegacyParticleGroup(Texture *t) {
	texture = t;
	ubo = nullptr;
#if USE_API_VULKAN
	ubo = new UniformBuffer(256);
	dset = rp_create_dset_fx(t, ubo);
#endif
}

LegacyParticleGroup::~LegacyParticleGroup() {
	for (auto *p: particles)
		delete p;
	for (auto *b: beams)
		delete b;
#if USE_API_VULKAN
	delete dset;
	delete ubo;
#endif
}

void LegacyParticleGroup::add(LegacyParticle *p) {
	if (p->type == BaseClass::Type::LEGACY_PARTICLE)
		particles.add(p);
	else if (p->type == BaseClass::Type::LEGACY_BEAM)
		beams.add((LegacyBeam*)p);
}

bool LegacyParticleGroup::unregister(LegacyParticle *p) {
	if (p->type == BaseClass::Type::LEGACY_PARTICLE) {
		foreachi (auto *pp, particles, i)
			if (pp == p) {
				//msg_write("  -> PARTICLE");
				particles.erase(i);
				return true;
			}
	} else if (p->type == BaseClass::Type::LEGACY_BEAM) {
		foreachi (auto *pp, beams, i)
			if (pp == p) {
				//msg_write("  -> BEAM");
				beams.erase(i);
				return true;
			}
	}
	return false;
}


void ParticleManager::add_legacy(LegacyParticle *p) {
	for (auto *g: legacy_groups)
		if (g->texture == p->texture) {
			g->add(p);
			return;
		}
	auto *gg = new LegacyParticleGroup(p->texture.get());
	gg->add(p);
	legacy_groups.add(gg);
}

bool ParticleManager::unregister_legacy(LegacyParticle *p) {
	for (auto *g: legacy_groups)
		if (g->unregister(p))
			return true;
	return false;
}

void ParticleManager::_delete_legacy(LegacyParticle *p) {
	if (unregister_legacy(p))
		delete(p);
}


void ParticleManager::register_particle_group(ParticleGroup *g) {
	particle_groups.add(g);
}

bool ParticleManager::unregister_particle_group(ParticleGroup *g) {
	int index = particle_groups.find(g);
	if (index >= 0) {
		particle_groups.erase(index);
		return true;
	}
	return false;
}

void ParticleManager::clear() {
	for (auto *g: legacy_groups)
		delete g;
	legacy_groups.clear();
}

static void iterate_legacy_particles(Array<LegacyParticle*> *particles, float dt) {
	foreachi (auto p, *particles, i) {
		p->pos += p->vel * dt;
		if (p->time_to_live >= 0) {
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
	for (auto g: legacy_groups) {
		iterate_legacy_particles(&g->particles, dt);
		iterate_legacy_particles((Array<LegacyParticle*>*)&g->beams, dt);
	}
	for (auto g: particle_groups)
		g->on_iterate(dt);
}

void ParticleManager::shift_all(const vec3 &dpos) {
	for (auto g: legacy_groups) {
		for (auto *p: g->particles)
			p->pos += dpos;
		for (auto *p: g->beams)
			p->pos += dpos;
	}
}


