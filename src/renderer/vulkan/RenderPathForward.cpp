/*
 * RenderPathForward.cpp
 *
 *  Created on: Jan 19, 2020
 *      Author: michi
 */

#if HAS_LIB_VULKAN

#include "RenderPathForward.h"
#include "Renderer.h"
#include "ShadowMapRenderer.h"
#include "../lib/vulkan/vulkan.h"
#include "../world/world.h"
#include "../world/camera.h"
#include "../fx/Light.h"
#include "../fx/Particle.h"
#include "../helper/PerformanceMonitor.h"
#include "../gui/Picture.h"

extern vulkan::Texture *tex_white;
extern vulkan::Texture *tex_black;

RenderPathForward::RenderPathForward(RendererVulkan *r, PerformanceMonitor *pm) : RenderPathVulkan(r, pm, "forward/3d-shadow.shader", "forward/3d-fx.shader") {

	// emission and directional sun pass
	shader_base = vulkan::Shader::load("forward/3d-base.shader");
	pipeline_base = new vulkan::Pipeline(shader_base, renderer->default_render_pass(), 0, 1);

	// additive light pass
	shader_light = vulkan::Shader::load("forward/3d-light.shader");
	pipeline_light = new vulkan::Pipeline(shader_light, renderer->default_render_pass(), 0, 1);
	pipeline_light->set_blend(VK_BLEND_FACTOR_SRC_COLOR, VK_BLEND_FACTOR_ONE);
	pipeline_light->set_z(true, false);
	pipeline_light->rebuild();


	// fog
/*	msg_write("a");
	shader_fog = vulkan::Shader::load("forward/2d-fog.shader");
	msg_write("a2");
	pipeline_fog = new vulkan::Pipeline(shader_fog, renderer->default_render_pass(), 0, 1);
	msg_write("a3");
	pipeline_fog->set_blend(VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
	pipeline_fog->set_z(false, false);
	pipeline_fog->rebuild();
	msg_write("a4");
	ubo_fog = new vulkan::UniformBuffer(256);
	dset_fog = new vulkan::DescriptorSet({ubo_fog, world.ubo_light, world.ubo_fog}, {r->depth_buffer()});
	msg_write("a5");*/
}

RenderPathForward::~RenderPathForward() {
	delete shader_base;
	delete pipeline_base;

	delete shader_light;
	delete pipeline_light;

	/*delete shader_fog;
	delete pipeline_fog;
	delete dset_fog;*/
}

void RenderPathForward::render_fx(vulkan::CommandBuffer *cb, RendererVulkan *r) {
	cb->set_pipeline(pipeline_fx);
	cb->set_viewport(r->area());

	cb->push_constant(80, sizeof(color), &world.fog._color);
	float density0 = 0;
	cb->push_constant(96, 4, &density0);


	for (auto *g: world.particle_manager->groups) {
		g->dset->set({g->ubo, world.ubo_light, world.ubo_fog}, {g->texture});
		cb->bind_descriptor_set_dynamic(0, g->dset, {0});

		for (auto *p: g->particles) {
			matrix m = cam->m_all * matrix::translation(p->pos) * matrix::rotation_q(cam->ang) * matrix::scale(p->radius, p->radius, 1);
			cb->push_constant(0, sizeof(m), &m);
			cb->push_constant(64, sizeof(color), &p->col);
			if (world.fog.enabled) {
				float dist = (cam->pos - p->pos).length(); //(cam->m_view * p->pos).z;
				float fog_density = 1-exp(-dist / world.fog.distance);
				cb->push_constant(96, 4, &fog_density);
			}
			cb->draw(particle_vb);
		}
	}

}

void RenderPathForward::draw() {
	vulkan::wait_device_idle();
	perf_mon->tick(0);
	light_cam->pos = vector(0,1000,0);
	light_cam->ang = quaternion::rotation_v(vector(pi/2, 0, 0));
	light_cam->zoom = 2;
	light_cam->min_depth = 50;
	light_cam->max_depth = 10000;
	light_cam->set_view(1.0f);
	world.sun->proj = light_cam->m_all;

	prepare_all(shadow_renderer, light_cam);
	render_into_shadow(shadow_renderer);

	if (!renderer->start_frame())
		return;

	perf_mon->tick(1);

	foreachi (auto *l, world.lights, i)
		world.ubo_light->update_single(l, i);

	prepare_all(renderer, cam);
	auto r = renderer;
	auto cb = r->cb;
	cb->begin();
	cam->set_view(1.0f);

	/*dset_fog->set({ubo_fog, world.ubo_light, world.ubo_fog}, {r->depth_buffer()});


	UBOMatrices u;
	u.proj = matrix::translation(vector(-1,-1,0)) * matrix::scale(2,2,1);
	u.view = matrix::ID;
	u.model = matrix::ID;
	ubo_fog->update(&u);*/


	r->default_render_pass()->clear_color[0] = world.background; // emission
	cb->begin_render_pass(r->default_render_pass(), r->current_frame_buffer());



	// emission + sun
	cb->set_pipeline(pipeline_base);
	cb->set_viewport(r->area());

	draw_world(cb, 0);
	perf_mon->tick(2);


	// light passes
	foreachi (auto *l, world.lights, light_index) {
		if (l == world.sun)
			continue;
		vulkan::wait_device_idle();
		cb->set_pipeline(pipeline_light);
		cb->set_viewport(r->area());

		draw_world(cb, light_index);
		perf_mon->tick(2);
	}



/*	cb->set_pipeline(pipeline_fog);

	cb->set_viewport(r->area());
	//cb->push_constant(0, 12, &cam->pos);

	cb->bind_descriptor_set(0, dset_fog);
	cb->draw(Picture::vertex_buffer);*/



	render_fx(cb, r);



	gui::render(cb, r->area());



	cb->end_render_pass();
	cb->end();


	renderer->end_frame();
	perf_mon->tick(5);
}


vulkan::DescriptorSet *RenderPathForward::rp_create_dset(const Array<vulkan::Texture*> &tex, vulkan::UniformBuffer *ubo) {
	if (tex.num == 3)
		return new vulkan::DescriptorSet({ubo, world.ubo_light, world.ubo_fog}, {tex[0], tex[1], tex[2], shadow_renderer->depth_buffer});
	else
		return new vulkan::DescriptorSet({ubo, world.ubo_light, world.ubo_fog}, {tex[0], tex_white, tex_black, shadow_renderer->depth_buffer});
}

vulkan::DescriptorSet *RenderPathForward::rp_create_dset_fx(vulkan::Texture *tex, vulkan::UniformBuffer *ubo) {
	return new vulkan::DescriptorSet({ubo, world.ubo_light, world.ubo_fog}, {tex});
}

#endif
