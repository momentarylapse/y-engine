/*
 * Particle.h
 *
 *  Created on: 08.01.2020
 *      Author: michi
 */

#pragma once

#include "../graphics-fwd.h"
#include "../lib/base/base.h"
#include "../lib/base/pointer.h"
#include "../lib/math/vec3.h"
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

	Particle(const vec3 &pos, float r, shared<Texture> tex, float ttl);
	virtual ~Particle();

	void __init__(const vec3 &pos, float r, shared<Texture> tex, float ttl);
	void __delete__() override;
	virtual void on_iterate(float dt) {}

	vec3 pos;
	vec3 vel;
	color col;
	rect source;
	shared<Texture> texture;
	float radius;
	float time_to_live;
	bool suicidal;
	bool enabled;
};

