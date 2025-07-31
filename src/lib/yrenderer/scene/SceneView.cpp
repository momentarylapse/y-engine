//
// Created by michi on 04.11.23.
//

#include "SceneView.h"
#include <world/World.h>
#include <world/Light.h>
#include <lib/base/iter.h>
#include <y/ComponentManager.h>
#include <lib/os/time.h>

namespace yrenderer {

void SceneView::choose_lights() {
	lights.clear();
	auto& all_lights = ComponentManager::get_list_family<Light>();
	for (auto l: all_lights) {
		if (l->enabled)
			lights.add(l);
	}
}

void SceneView::choose_shadows() {
	shadow_indices.clear();
	for (auto&& [i,l]: enumerate(lights)) {
		l->light.shadow_index = -1;
		if (l->allow_shadow and shadow_indices.num == 0) {
			l->light.shadow_index = shadow_indices.num;
			shadow_indices.add(i);
		}
	}
}

}
