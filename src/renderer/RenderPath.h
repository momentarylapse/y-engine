/*
 * RenderPath.h
 *
 *  Created on: Jan 19, 2020
 *      Author: michi
 */

#ifndef SRC_RENDERER_RENDERPATH_H_
#define SRC_RENDERER_RENDERPATH_H_

#include "../lib/math/math.h"

#if HAS_LIB_VULKAN
namespace vulkan {
	class Shader;
	class Pipeline;
	class CommandBuffer;
	class VertexBuffer;
	class DescriptorSet;
	class UniformBuffer;
	class Texture;
}
#endif
class Renderer;
class RendererVulkan;
class ShadowMapRenderer;
class PerformanceMonitor;
class World;
class Camera;
class matrix;
class vector;
class quaternion;

namespace nix {
	class Shader;
}

matrix mtr(const vector &t, const quaternion &a);



struct UBOMatrices {
	alignas(16) matrix model;
	alignas(16) matrix view;
	alignas(16) matrix proj;
};

struct UBOFog {
	alignas(16) color col;
	alignas(16) float distance;
};

class RenderPath {
public:
	RenderPath() {}
	virtual ~RenderPath() {}
	virtual void draw() = 0;
	virtual void start_frame() = 0;
	virtual void end_frame() = 0;

	nix::Shader *shader_3d = nullptr;
	nix::Shader *shader_3d_multi = nullptr;
	nix::Shader *shader_2d = nullptr;

	// dynamic resolution scaling
	float resolution_scale_x = 1.0f;
	float resolution_scale_y = 1.0f;
};


#if HAS_LIB_VULKAN
class RenderPathVulkan : public RenderPath {
public:
	RenderPathVulkan(RendererVulkan *renderer, PerformanceMonitor *perf_mon, const string &shadow_shader_filename, const string &fx_shader_filename);
	virtual ~RenderPathVulkan();


	void prepare_all(Renderer *r, Camera *c);
	void draw_world(vulkan::CommandBuffer *cb, int light_index);
	//void render_fx(vulkan::CommandBuffer *cb, Renderer *r);

	void render_into_shadow(ShadowMapRenderer *r);


	vulkan::Pipeline *pipeline_fx;
	vulkan::VertexBuffer *particle_vb;

	RendererVulkan *renderer;
	ShadowMapRenderer *shadow_renderer;
	PerformanceMonitor *perf_mon;

	Camera *light_cam;
	void pick_shadow_source();


	virtual vulkan::DescriptorSet *rp_create_dset(const Array<vulkan::Texture*> &tex, vulkan::UniformBuffer *ubo) = 0;
	virtual vulkan::DescriptorSet *rp_create_dset_fx(vulkan::Texture *tex, vulkan::UniformBuffer *ubo) = 0;
};
#endif

#endif /* SRC_RENDERER_RENDERPATH_H_ */
