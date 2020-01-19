/*
 * Particle.cpp
 *
 *  Created on: 08.01.2020
 *      Author: michi
 */

#include "Particle.h"
#include "../lib/vulkan/vulkan.h"
#include <iostream>

vulkan::Shader *shader_fx;

vulkan::DescriptorSet *rp_create_dset_fx(vulkan::Texture *tex, vulkan::UniformBuffer *ubo);

Particle::Particle(const vector &p, float r, vulkan::Texture *t) {
	pos = p;
	col = White;
	radius = r;
	texture = t;
}

Particle::~Particle() {
}

void Particle::__init__(const vector &p, float r, vulkan::Texture *t) {
	new(this) Particle(p, r, t);
}



ParticleGroup::ParticleGroup(vulkan::Texture *t) {
	texture = t;
	ubo = new vulkan::UniformBuffer(256);
	dset = rp_create_dset_fx(t, ubo);
}

ParticleGroup::~ParticleGroup() {
	for (auto *p: particles)
		delete p;
	delete dset;
	delete ubo;
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

