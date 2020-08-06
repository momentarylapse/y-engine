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


namespace nix {
	class Texture;
	class UniformBuffer;
	class Shader;
}


/*struct ParticlePushData {
	alignas(16) matrix m;
	color fog_color;
	float fog_distance;
};*/

class Particle {
public:
	Particle(const vector &pos, float r, nix::Texture *tex);
	virtual ~Particle();

	void __init__(const vector &pos, float r, nix::Texture *tex);

	vector pos;
	color col;
	float radius;
	rect source;
	nix::Texture *texture;
};

class ParticleGroup {
public:
	ParticleGroup(nix::Texture *t);
	~ParticleGroup();
	Array<Particle*> particles;
	nix::Texture *texture;
	nix::UniformBuffer *ubo;
	//DescriptorSet *dset;
};

class ParticleManager {
public:
	Array<ParticleGroup*> groups;
	void add(Particle *o);
	void clear();
};

#endif /* SRC_FX_PARTICLE_H_ */
