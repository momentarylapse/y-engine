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

class ParticleGroup {
public:
	ParticleGroup(nix::Texture *t);
	~ParticleGroup();
	Array<Particle*> particles;
	Array<Beam*> beams;
	void add(Particle *p);
	nix::Texture *texture;
	nix::UniformBuffer *ubo;
	//DescriptorSet *dset;
};

class ParticleManager {
public:
	Array<ParticleGroup*> groups;
	void add(Particle *o);
	void clear();
	void iterate(float dt);
};



#endif /* SRC_FX_PARTICLEMANAGER_H_ */
