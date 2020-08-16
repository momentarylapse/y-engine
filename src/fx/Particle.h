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
	class Shader;
}


/*struct ParticlePushData {
	alignas(16) matrix m;
	color fog_color;
	float fog_distance;
};*/

class Particle : public VirtualBase {
public:

	enum class Type {
		PARTICLE,
		BEAM
	};

	Particle(const vector &pos, float r, nix::Texture *tex, float ttl);
	virtual ~Particle();

	void __init__(const vector &pos, float r, nix::Texture *tex, float ttl);
	void __delete__() override;
	virtual void on_iterate(float dt) {}

	Type type;
	vector pos;
	vector vel;
	color col;
	rect source;
	nix::Texture *texture;
	float radius;
	float time_to_live;
	bool suicidal;
};


#endif /* SRC_FX_PARTICLE_H_ */
