/*
 * Particle.h
 *
 *  Created on: 08.01.2020
 *      Author: michi
 */

#ifndef SRC_FX_PARTICLE_H_
#define SRC_FX_PARTICLE_H_

#include "../lib/base/base.h"
#include "../lib/math/math.h"

namespace vulkan {
	class Texture;
	class DescriptorSet;
	class UniformBuffer;
}

/*struct ParticlePushData {
	alignas(16) matrix m;
	color fog_color;
	float fog_distance;
};*/

class Particle {
public:
	Particle(const vector &pos, float r, vulkan::Texture *tex);
	virtual ~Particle();

	void __init__(const vector &pos, float r, vulkan::Texture *tex);

	vector pos;
	color col;
	float radius;
	vulkan::Texture *texture;
};

class ParticleGroup {
public:
	ParticleGroup(vulkan::Texture *t);
	~ParticleGroup();
	Array<Particle*> particles;
	vulkan::Texture *texture;
	vulkan::UniformBuffer *ubo;
	vulkan::DescriptorSet *dset;
};

class ParticleManager {
public:
	Array<ParticleGroup*> groups;
	void add(Particle *o);
	void clear();
};

#endif /* SRC_FX_PARTICLE_H_ */
