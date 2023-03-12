/*
 * ParticleManager.h
 *
 *  Created on: Aug 9, 2020
 *      Author: michi
 */

#pragma once

#include "../graphics-fwd.h"
#include "../lib/base/base.h"

class Particle;
class Beam;
class vec3;

class ParticleGroup {
public:
	ParticleGroup(Texture *t);
	~ParticleGroup();
	Array<Particle*> particles;
	Array<Beam*> beams;
	void add(Particle *p);
	bool unregister(Particle *p);
	Texture *texture;
	UniformBuffer *ubo;
	//DescriptorSet *dset;
};

class ParticleManager {
public:
	Array<ParticleGroup*> groups;
	void add(Particle *p);
	void _delete(Particle *p);
	bool unregister(Particle *p);
	void clear();
	void iterate(float dt);
	void shift_all(const vec3 &dpos);
};


