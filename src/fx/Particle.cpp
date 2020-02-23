/*
 * Particle.cpp
 *
 *  Created on: 08.01.2020
 *      Author: michi
 */

#include "Particle.h"
#include <iostream>

nix::Shader *shader_fx;

//DescriptorSet *rp_create_dset_fx(Texture *tex, UniformBuffer *ubo);

Particle::Particle(const vector &p, float r, nix::Texture *t) {
	pos = p;
	col = White;
	radius = r;
	texture = t;
}

Particle::~Particle() {
}

void Particle::__init__(const vector &p, float r, nix::Texture *t) {
	new(this) Particle(p, r, t);
}



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
#if USE_API_VULKAN
	delete dset;
	delete ubo;
#endif
}



void ParticleManager::add(Particle *p) {
	for (auto *g: groups)
		if (g->texture == p->texture) {
			g->particles.add(p);
			return;
		}
	auto *gg = new ParticleGroup(p->texture);
	gg->particles.add(p);
	groups.add(gg);
}

void ParticleManager::clear() {
	for (auto *g: groups)
		delete g;
	groups.clear();
}

