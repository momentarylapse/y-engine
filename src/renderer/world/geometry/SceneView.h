//
// Created by michi on 04.11.23.
//

#ifndef Y_SCENEVIEW_H
#define Y_SCENEVIEW_H

#include <lib/base/base.h>
#include <lib/base/pointer.h>
#include <lib/math/mat4.h>
#include <graphics-fwd.h>

class Light;
class Camera;
struct UBOLight;
struct XTerrainVBUpdater;
class TerrainUpdateThread;
struct RayTracingData;

struct SceneView {
	Camera *cam; // the "owning" camera - might use a different perspective for rendering (e.g. cubemap)
	Array<DepthBuffer*> shadow_maps;
	shared<CubeMap> cube_map;
	int shadow_index = -1;
	Array<Light*> lights;
	owned<UniformBuffer> surfel_buffer;
	int num_surfels = 0;
	RayTracingData* ray_tracing_data = nullptr;

	void choose_lights();

	//void prepare_lights(float shadow_box_size, UniformBuffer* ubo_light);


	void check_terrains(const vec3& cam_pos);

	//TerrainUpdateThread* terrain_update_thread = nullptr;
	Array<XTerrainVBUpdater*> updater;
};


#endif //Y_SCENEVIEW_H
