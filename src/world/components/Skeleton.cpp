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

Model* fancy_copy(Model *orig);

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
	for (int i=0; i<bone.num; i++) {
		bone[i].rest_pos = get_bone_rest_pos(i);
		bone[i].cur_pos = bone[i].rest_pos;
		bone[i].cur_ang = quaternion::ID;
		if (bone[i].model) {
			bone[i].model = fancy_copy(bone[i].model->_template->model);//ModelManager::load(bone[i].model->filename().no_ext());
		}
	}
}

// non-animated state
vector Skeleton::get_bone_rest_pos(int index) const {
	auto &b = bone[index];
	if (b.parent >= 0)
		return b.delta_pos + get_bone_rest_pos(b.parent);
	return b.delta_pos;
}

