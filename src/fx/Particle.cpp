/*
 * Particle.cpp
 *
 *  Created on: 08.01.2020
 *      Author: michi
 */

#include "Particle.h"
#include "../graphics-impl.h"

Shader *shader_fx;

//DescriptorSet *rp_create_dset_fx(Texture *tex, UniformBuffer *ubo);

LegacyParticle::LegacyParticle(const vec3 &p, float r, shared<Texture> t, float ttl) : BaseClass(Type::PARTICLE) {
	pos = p;
	vel = vec3::ZERO;
	col = White;
	radius = r;
	time_to_live = -1;
	texture = t;
	source = rect::ID;
	time_to_live = ttl;
	suicidal = ttl > 0;
	enabled = true;
}

LegacyParticle::~LegacyParticle() {
}

void LegacyParticle::__init__(const vec3 &p, float r, shared<Texture> t, float ttl) {
	new(this) LegacyParticle(p, r, t, ttl);
}

void LegacyParticle::__delete__() {
	this->LegacyParticle::~LegacyParticle();
}

