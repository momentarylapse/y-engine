/*
 * Light.h
 *
 *  Created on: 02.01.2020
 *      Author: michi
 */

#pragma once

#include "../lib/base/base.h"
#include "../lib/math/vector.h"
#include "../lib/math/matrix.h"
#include "../lib/image/color.h"
#include "Entity3D.h"

class Camera;

struct UBOLight {
	alignas(16) matrix proj; // view -> texture
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

class Light : public Entity3D {
public:
	Light(const vector &p, const quaternion &q, const color &c, float r, float t);
	void __init_parallel__(const quaternion &ang, const color &c);
	void __init_spherical__(const vector &p, const color &c, float r);
	void __init_cone__(const vector &p, const quaternion &ang, const color &c, float r, float t);

	void set_direction(const vector &dir);

	void update(Camera *cam, float shadow_box_size, bool using_view_space);

	UBOLight light;
	bool enabled;
	bool allow_shadow;
	bool user_shadow_control;
	float user_shadow_theta;
	matrix shadow_projection; // world -> texture

	LightType type;
};


