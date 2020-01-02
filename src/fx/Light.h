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
#include "../lib/image/color.h"


class Light {
public:
	Light(const vector &p, const vector &d, const color &c, float r, float t);
	void __init_parallel__(const vector &d, const color &c);
	void __init_spherical__(const vector &p, const color &c, float r);
	void __init_cone__(const vector &p, const vector &d, const color &c, float r, float t);
	vector pos, dir;
	color col;
	bool enabled;
	float radius, theta;
};



#endif /* SRC_FX_LIGHT_H_ */
