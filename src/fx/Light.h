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

/*namespace vulkan {
	class DescriptorSet;
	class UniformBuffer;
}*/

struct UBOLight {
	alignas(16) matrix proj;
	alignas(16) vector pos;
	alignas(16) vector dir;
	alignas(16) color col;
	alignas(16) float radius;
	float theta, harshness;
};

class Light : public UBOLight {
public:
	Light(const vector &p, const vector &d, const color &c, float r, float t);
	~Light();
	void __init_parallel__(const vector &d, const color &c);
	void __init_spherical__(const vector &p, const color &c, float r);
	void __init_cone__(const vector &p, const vector &d, const color &c, float r, float t);
	//vector pos, dir;
	//color col;
	bool enabled;
	//float radius, theta, harshness;
//	vulkan::UniformBuffer *ubo;
//	vulkan::DescriptorSet *dset;
};



#endif /* SRC_FX_LIGHT_H_ */
