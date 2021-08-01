/*
 * Skeleton.h
 *
 *  Created on: Jul 17, 2021
 *      Author: michi
 */

#pragma once

#include "../../y/Component.h"
#include "../Entity3D.h"

class Model;
class Path;


class Skeleton : public Component {
public:
	Skeleton();
	virtual ~Skeleton();

	void on_init() override;

	Array<Entity3D> bones; // pos relative to parent entity (skeleton)
	Array<int> parent;
	Array<vector> pos0; // relative to parent entity (skeleton)
	Array<vector> dpos; // relative to parent bone
	Array<Path> filename;

	vector _calc_bone_rest_pos(int index) const;
	void _cdecl set_bone_model(int index, Model *sub);


	static const kaba::Class *_class;
};
