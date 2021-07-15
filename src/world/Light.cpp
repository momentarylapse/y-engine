/*
 * Light.cpp
 *
 *  Created on: 02.01.2020
 *      Author: michi
 */

#include "../world/Light.h"

#include <stdio.h>

Light::Light(const vector &p, const quaternion &q, const color &c, float r, float t) : Entity3D(Type::LIGHT) {
	pos = p;
	ang = q;
	light.pos = p;
	light.dir = q * vector::EZ;
	light.col = c;
	light.radius = r;
	light.theta = t;
	light.harshness = 0.8f;
	if (light.radius >= 0)
		light.harshness = 1;
	enabled = true;
	allow_shadow = false;
	user_shadow_control = false;
	user_shadow_theta = -1;
	type = LightType::DIRECTIONAL;
	if (light.radius > 0) {
		if (light.theta > 0)
			type = LightType::CONE;
		else
			type = LightType::POINT;
	}
}


void Light::set_direction(const vector &dir) {
	ang = quaternion::rotation(dir.dir2ang());
}

void Light::__init_parallel__(const quaternion &ang, const color &c) {
	new(this) Light(v_0, ang, c, -1, -1);
}
void Light::__init_spherical__(const vector &p, const color &c, float r) {
	new(this) Light(p, quaternion::ID, c, r, -1);
}
void Light::__init_cone__(const vector &p, const quaternion &ang, const color &c, float r, float t) {
	new(this) Light(p, ang, c, r, t);
}


