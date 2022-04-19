/*
 * Particle.cpp
 *
 *  Created on: 08.01.2020
 *      Author: michi
 */

#include "Particle.h"

Shader *shader_fx;

//DescriptorSet *rp_create_dset_fx(Texture *tex, UniformBuffer *ubo);

Particle::Particle(const vector &p, float r, Texture *t, float ttl) : BaseClass(Type::PARTICLE) {
	pos = p;
	vel = vector::ZERO;
	col = White;
	radius = r;
	time_to_live = -1;
	texture = t;
	source = rect::ID;
	time_to_live = ttl;
	suicidal = ttl > 0;
	enabled = true;
}

Particle::~Particle() {
}

void Particle::__init__(const vector &p, float r, Texture *t, float ttl) {
	new(this) Particle(p, r, t, ttl);
}

void Particle::__delete__() {
	this->Particle::~Particle();
}

