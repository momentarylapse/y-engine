/*
 * DeferredRenderer.h
 *
 *  Created on: 06.01.2020
 *      Author: michi
 */

#ifndef SRC_RENDERER_DEFERREDRENDERER_H_
#define SRC_RENDERER_DEFERREDRENDERER_H_

#include "Renderer.h"

class rect;
class Light;
class Camera;
class ShadowMapRenderer;
class GBufferRenderer;

class DeferredRenderer : public Renderer {
public:
	DeferredRenderer(Renderer *_output_renderer);
	~DeferredRenderer() override;

	void _create_dynamic_data();
	void _destroy_dynamic_data();
	void resize(int w, int h);

	vector project_pixel(const vector &v);
	float projected_sphere_radius(vector &v, float r);
	rect light_rect(Light *l);
	void set_light(Light *ll);
	void draw_from_gbuf_single(vulkan::CommandBuffer *cb, vulkan::Pipeline *pip, vulkan::DescriptorSet *dset, const rect &r);
	void render_out(vulkan::CommandBuffer *cb, Renderer *ro);

	Renderer *output_renderer;
	GBufferRenderer *gbuf_ren;
	ShadowMapRenderer *shadow_renderer;

	vulkan::UBOWrapper *ubo_x1;
	vulkan::DescriptorSet *dset_x1;

	vulkan::Shader *shader_merge_base;
	vulkan::Shader *shader_merge_light;
	vulkan::Shader *shader_merge_light_shadow;
	vulkan::Shader *shader_merge_fog;


	vulkan::Pipeline *pipeline_x1;
	vulkan::Pipeline *pipeline_x2;
	vulkan::Pipeline *pipeline_x2s;
	vulkan::Pipeline *pipeline_x3;

	Camera *light_cam;
	void pick_shadow_source();
};



#endif /* SRC_RENDERER_DEFERREDRENDERER_H_ */
