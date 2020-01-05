/*
 * Light.cpp
 *
 *  Created on: 02.01.2020
 *      Author: michi
 */

#include "Light.h"
#include "../lib/vulkan/vulkan.h"
#include <stdio.h>

Light::Light(const vector &p, const vector &d, const color &c, float r, float t) {
	pos = p;
	dir = d;
	col = c;
	radius = r;
	theta = t;
	harshness = 0.8f;
	enabled = true;

	ubo = new vulkan::UBOWrapper(sizeof(UBOLight));
	dset = nullptr;
}

Light::~Light() {
	if (ubo)
		delete ubo;
	if (dset)
		delete dset;
}

void Light::__init_parallel__(const vector &d, const color &c) {
	new(this) Light(v_0, d, c, -1, -1);
}
void Light::__init_spherical__(const vector &p, const color &c, float r) {
	new(this) Light(p, v_0, c, r, -1);
}
void Light::__init_cone__(const vector &p, const vector &d, const color &c, float r, float t) {
	new(this) Light(p, d, c, r, t);
}


