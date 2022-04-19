/*
 * Particle.h
 *
 *  Created on: 08.01.2020
 *      Author: michi
 */

#pragma once

#include "../graphics-fwd.h"
#include "../lib/base/base.h"
#include "../lib/math/vector.h"
#include "../lib/math/rect.h"
#include "../lib/image/color.h"
#include "../y/BaseClass.h"


/*struct ParticlePushData {
	alignas(16) matrix m;
	color fog_color;
	float fog_distance;
};*/

class Particle : public BaseClass {
public:

	Particle(const vector &pos, float r, Texture *tex, float ttl);
	virtual ~Particle();

	void __init__(const vector &pos, float r, Texture *tex, float ttl);
	void __delete__() override;
	virtual void on_iterate(float dt) {}

	vector pos;
	vector vel;
	color col;
	rect source;
	Texture *texture;
	float radius;
	float time_to_live;
	bool suicidal;
	bool enabled;
};

