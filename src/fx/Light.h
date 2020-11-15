/*
 * Light.h
 *
 *  Created on: 02.01.2020
 *      Author: michi
 */

#ifndef SRC_FX_LIGHT_H_
#define SRC_FX_LIGHT_H_

#include "../lib/base/base.h"
#include "../lib/math/vector.h"
#include "../lib/math/matrix.h"
#include "../lib/image/color.h"
#include "../y/Entity.h"


struct UBOLight {
	alignas(16) matrix proj;
	alignas(16) vector pos;
	alignas(16) vector dir;
	alignas(16) color col;
	alignas(16) float radius;
	float theta, harshness;
};

enum class LightType {
	DIRECTIONAL,
	POINT,
	CONE
};

class Light : public Entity {
public:
	Light(const vector &p, const vector &d, const color &c, float r, float t);
	void __init_parallel__(const vector &d, const color &c);
	void __init_spherical__(const vector &p, const color &c, float r);
	void __init_cone__(const vector &p, const vector &d, const color &c, float r, float t);
	UBOLight light;
	bool enabled;
	bool allow_shadow;
	bool user_shadow_control;
	float user_shadow_theta;

	LightType type;
};



#endif /* SRC_FX_LIGHT_H_ */
