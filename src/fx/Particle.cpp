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

Particle::Particle(const vector &p, float r, nix::Texture *t, float ttl) {
	pos = p;
	vel = vector::ZERO;
	col = White;
	radius = r;
	time_to_live = -1;
	texture = t;
	source = rect::ID;
	time_to_live = ttl;
	suicidal = ttl > 0;
}

Particle::~Particle() {
}

void Particle::__init__(const vector &p, float r, nix::Texture *t, float ttl) {
	new(this) Particle(p, r, t, ttl);
}

void Particle::__delete__() {
	this->~Particle();
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

void ParticleManager::iterate(float dt) {
	for (auto g: groups)
		foreachi (auto p, g->particles, i) {
			p->pos += p->vel * dt;
			if (p->suicidal) {
				p->time_to_live -= dt;
				if (p->time_to_live < 0) {
					g->particles.erase(i);
					delete p;
					i --;
					continue;
				}
			}
			p->on_iterate(dt);
		}
}
