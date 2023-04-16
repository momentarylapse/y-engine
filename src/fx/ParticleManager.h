/*
 * ParticleManager.h
 *
 *  Created on: Aug 9, 2020
 *      Author: michi
 */

#pragma once

#include "../graphics-fwd.h"
#include "../lib/base/base.h"

class LegacyParticle;
class LegacyBeam;
class vec3;

class LegacyParticleGroup {
public:
	LegacyParticleGroup(Texture *t);
	~LegacyParticleGroup();
	Array<LegacyParticle*> particles;
	Array<LegacyBeam*> beams;
	void add(LegacyParticle *p);
	bool unregister(LegacyParticle *p);
	Texture *texture;
	UniformBuffer *ubo;
};

class ParticleManager {
public:
	Array<LegacyParticleGroup*> legacy_groups;
	void add_legacy(LegacyParticle *p);
	void _delete_legacy(LegacyParticle *p);
	bool unregister_legacy(LegacyParticle *p);
	void clear();
	void iterate(float dt);
	void shift_all(const vec3 &dpos);
};


