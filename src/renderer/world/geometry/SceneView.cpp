//
// Created by michi on 04.11.23.
//

#include "SceneView.h"
#include <graphics-impl.h>
#include <world/World.h>
#include <world/Light.h>
#include <world/Terrain.h>
#include <helper/PerformanceMonitor.h>
#include <y/ComponentManager.h>
#include <lib/threads/Thread.h>
#include <atomic>


void SceneView::prepare_lights(float shadow_box_size) {
	lights.clear();
	shadow_index = -1;
	auto& all_lights = ComponentManager::get_list_family<Light>();
	for (auto l: all_lights) {
		if (!l->enabled)
			continue;

		l->update(cam, shadow_box_size, true);

		if (l->allow_shadow) {
			shadow_index = lights.num;
			shadow_proj = l->shadow_projection;
		}
		lights.add(l->light);
	}
#ifdef USING_OPENGL
	ubo_light->update_array(lights);
#endif
	//rvd.ubo_light->update_part(&lights[0], 0, lights.num * sizeof(lights[0]));
}

class TerrainUpdateThread: public Thread {
public:
	void on_run() override {

	}

	std::atomic<Terrain*> terrain = nullptr;
};

void SceneView::check_terrains(const vec3& cam_pos) {
	auto& terrains = ComponentManager::get_list_family<Terrain>();
	if (terrains.num == 0)
		return;

	/*if (!terrain_update_thread) {
		terrain_update_thread = new TerrainUpdateThread();
		terrain_update_thread->run();
	}*/

	for (auto *t: terrains)
		t->prepare_draw(cam_pos);

}
