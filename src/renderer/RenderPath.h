/*
 * RenderPath.h
 *
 *  Created on: Jan 19, 2020
 *      Author: michi
 */

#ifndef SRC_RENDERER_RENDERPATH_H_
#define SRC_RENDERER_RENDERPATH_H_

#include "../lib/math/math.h"

namespace vulkan {
	class Shader;
	class Pipeline;
	class CommandBuffer;
	class VertexBuffer;
	class DescriptorSet;
	class UBOWrapper;
	class Texture;
}
class Renderer;
class ShadowMapRenderer;
class PerformanceMonitor;
class World;
class Camera;
class matrix;
class vector;
class quaternion;


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
	RenderPath(Renderer *renderer, PerformanceMonitor *perf_mon, const string &shadow_shader_filename);
	virtual ~RenderPath();

	virtual void draw() = 0;


	void prepare_all(Renderer *r, Camera *c);
	void draw_world(vulkan::CommandBuffer *cb);
	void render_fx(vulkan::CommandBuffer *cb, Renderer *r);

	void render_into_shadow(ShadowMapRenderer *r);


	vulkan::Pipeline *pipeline_fx;
	vulkan::VertexBuffer *particle_vb;

	Renderer *renderer;
	ShadowMapRenderer *shadow_renderer;
	PerformanceMonitor *perf_mon;

	Camera *light_cam;
	void pick_shadow_source();


	virtual vulkan::DescriptorSet *rp_create_dset(const Array<vulkan::Texture*> &tex, vulkan::UBOWrapper *ubo) = 0;
};

#endif /* SRC_RENDERER_RENDERPATH_H_ */
