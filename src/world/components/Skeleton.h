/*
 * Skeleton.h
 *
 *  Created on: Jul 17, 2021
 *      Author: michi
 */

#pragma once

#include "../../y/Component.h"
#include "../../lib/math/vector.h"
#include "../../lib/math/quaternion.h"
#include "../../lib/math/matrix.h"

class Model;


class Bone {
public:
	int parent;
	vector delta_pos;
	vector rest_pos;
	Model *model;
	// current skeletal data
	quaternion cur_ang;
	vector cur_pos;
	matrix dmatrix;
};


class Skeleton : public Component {
public:
	Skeleton();
	virtual ~Skeleton();

	void on_init() override;

	Array<Bone> bone;

	vector _cdecl get_bone_rest_pos(int index) const;
	void _cdecl set_bone_model(int index, Model *sub);


	static const kaba::Class *_class;
};
