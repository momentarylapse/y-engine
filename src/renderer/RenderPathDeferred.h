/*
 * RenderPathDeferred.h
 *
 *  Created on: 06.01.2020
 *      Author: michi
 */

#ifndef SRC_RENDERER_RENDERPATHDEFERRED_H_
#define SRC_RENDERER_RENDERPATHDEFERRED_H_

#include "RenderPath.h"

class rect;
class Light;
class Camera;
class ShadowMapRenderer;
class GBufferRenderer;

class RenderPathDeferred : public RenderPath {
public:
	RenderPathDeferred(Renderer *_output_renderer, PerformanceMonitor *perf_mon);
	~RenderPathDeferred() override;

	void draw() override;

	void _create_dynamic_data();
	void _destroy_dynamic_data();
	void resize(int w, int h);

	vector project_pixel(const vector &v);
	float projected_sphere_radius(vector &v, float r);
	rect light_rect(Light *l);
	void set_light(Light *ll);
	void draw_from_gbuf_single(vulkan::CommandBuffer *cb, vulkan::Pipeline *pip, vulkan::DescriptorSet *dset, const rect &r);
	void render_out(vulkan::CommandBuffer *cb, Renderer *ro);

	void render_all_from_deferred(Renderer *r);
	void render_into_gbuffer(GBufferRenderer *r);

	int width, height;
	Renderer *output_renderer;
	GBufferRenderer *gbuf_ren;

	vulkan::UniformBuffer *ubo_x1;
	vulkan::DescriptorSet *dset_x1;

	vulkan::Shader *shader_merge_base;
	vulkan::Shader *shader_merge_light;
	vulkan::Shader *shader_merge_light_shadow;
	vulkan::Shader *shader_merge_fog;


	vulkan::Pipeline *pipeline_x1;
	vulkan::Pipeline *pipeline_x2;
	vulkan::Pipeline *pipeline_x2s;
	vulkan::Pipeline *pipeline_x3;

	vulkan::DescriptorSet *rp_create_dset(const Array<vulkan::Texture*> &tex, vulkan::UniformBuffer *ubo) override;
};



#endif /* SRC_RENDERER_RENDERPATHDEFERRED_H_ */
