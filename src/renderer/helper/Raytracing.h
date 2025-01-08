//
// Created by Michael Ankele on 2025-01-08.
//

#ifndef RAYTRACING_H
#define RAYTRACING_H

#include <lib/base/base.h>
#include <lib/base/optional.h>
#include <lib/base/pointer.h>
#include <lib/math/vec2.h>
#include <lib/math/vec3.h>
#include <lib/math/mat4.h>
#include <lib/image/color.h>
#include "../../graphics-fwd.h"

class Model;
struct SceneView;

struct RayTracingData {
	owned<UniformBuffer> buffer_meshes;
	int num_meshes = 0;

	enum class Mode {
		NONE,
		COMPUTE,
		RTX
	} mode = Mode::NONE;

	struct MeshDescription {
		mat4 matrix;
		color albedo;
		color emission;
		int64 address_vertices;
		int64 address_indices;
		int num_triangles;
		int _a, _b, _c;
	};

#ifdef USING_VULKAN
	explicit RayTracingData(vulkan::Device *_device);

	void update_frame();



	struct ComputeModeData {
		vulkan::DescriptorPool *pool;
		DescriptorSet *dset;
		ComputePipeline *pipeline;
	} compute;

	struct RtxModeData {
		vulkan::DescriptorPool *pool;
		vulkan::DescriptorSet *dset;
		vulkan::RayPipeline *pipeline;
		vulkan::AccelerationStructure *tlas = nullptr;
		Array<vulkan::AccelerationStructure*> blas;
	} rtx;
#endif
};

struct RayHitInfo {
	Model* m;
	vec2 fg;
	vec3 p, n;
	int index;
	float t;
};

struct RayRequest {
	vec3 p0, p1;
};

void rt_setup(SceneView& scene_view);
void rt_update_frame(SceneView& scene_view);

Array<base::optional<RayHitInfo>> vtrace(SceneView& scene_view, const Array<RayRequest>& requests);


#endif //RAYTRACING_H
