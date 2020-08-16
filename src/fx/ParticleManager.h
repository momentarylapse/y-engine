/*
 * ParticleManager.h
 *
 *  Created on: Aug 9, 2020
 *      Author: michi
 */

#ifndef SRC_FX_PARTICLEMANAGER_H_
#define SRC_FX_PARTICLEMANAGER_H_

#include "../lib/base/base.h"

namespace nix {
	class Texture;
	class UniformBuffer;
}
class Particle;
class Beam;
class vector;

class ParticleGroup {
public:
	ParticleGroup(nix::Texture *t);
	~ParticleGroup();
	Array<Particle*> particles;
	Array<Beam*> beams;
	void add(Particle *p);
	bool unregister(Particle *p);
	nix::Texture *texture;
	nix::UniformBuffer *ubo;
	//DescriptorSet *dset;
};

class ParticleManager {
public:
	Array<ParticleGroup*> groups;
	void add(Particle *p);
	bool try_delete(Particle *p);
	bool unregister(Particle *p);
	void clear();
	void iterate(float dt);
	void shift_all(const vector &dpos);
};



#endif /* SRC_FX_PARTICLEMANAGER_H_ */
