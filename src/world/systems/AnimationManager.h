//
// Created by michi on 3/28/26.
//

#pragma once

#include "../../ecs/System.h"


class AnimationManager : public System {
public:
	void on_iterate(float dt) override;
	void on_add_component(const EntityMessageParams &params) override;
	void on_remove_component(const EntityMessageParams &params) override;

	static const kaba::Class* _class;
};

