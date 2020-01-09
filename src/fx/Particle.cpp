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
	dset = new vulkan::DescriptorSet(shader_fx->descr_layouts[0], {}, {t, t});
}

ParticleGroup::~ParticleGroup() {
	for (auto *p: particles)
		delete p;
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

