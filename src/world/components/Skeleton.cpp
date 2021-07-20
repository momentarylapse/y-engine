/*
 * Skeleton.cpp
 *
 *  Created on: Jul 17, 2021
 *      Author: michi
 */

#include "Skeleton.h"
#include "../Model.h"
#include "../../y/Entity.h"
#include "../ModelManager.h"

const kaba::Class *Skeleton::_class = nullptr;

Skeleton::Skeleton() {
}

Skeleton::~Skeleton() {

	if (Model::AllowDeleteRecursive) {
		// delete sub models
		for (Bone &b: bone)
			if (b.model)
				delete b.model;
	}
}

void Skeleton::on_init() {
	auto m = owner->get_component<Model>();

	bone = m->_template->skeleton->bone;

	// skeleton
	/*m->bone = bone;
	for (int i=0;i<bone.num;i++)
		if (bone[i].model)
			m->bone[i].model = nullptr;//CopyModel(bone[i].model, allow_script_init);*/



	// skeleton
	for (int i=0; i<bone.num; i++) {
		bone[i].rest_pos = get_bone_rest_pos(i);
	}
}

// non-animated state
vector Skeleton::get_bone_rest_pos(int index) const {
	auto &b = bone[index];
	if (b.parent >= 0)
		return b.delta_pos + get_bone_rest_pos(b.parent);
	return b.delta_pos;
}

