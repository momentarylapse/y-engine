/*
 * WorldRendererVulkanForward.cpp
 *
 *  Created on: Nov 18, 2021
 *      Author: michi
 */

#include "WorldRendererVulkanForward.h"
#ifdef USING_VULKAN
#include "pass/ShadowRendererVulkan.h"
#include "../../graphics-impl.h"
#include "../base.h"
#include "../../lib/os/msg.h"

#include "../../helper/PerformanceMonitor.h"
#include "../../helper/ResourceManager.h"
#include "../../helper/Scheduler.h"
#include "../../plugins/PluginManager.h"
#include "../../gui/gui.h"
#include "../../gui/Node.h"
#include "../../gui/Picture.h"
#include "../../gui/Text.h"
#include "../../fx/Particle.h"
#include "../../fx/Beam.h"
#include "../../fx/ParticleManager.h"
#include "../../world/Camera.h"
#include "../../world/Light.h"
#include "../../world/Material.h"
#include "../../world/Model.h"
#include "../../world/Object.h" // meh
#include "../../world/Terrain.h"
#include "../../world/World.h"
#include "../../y/Entity.h"
#include "../../Config.h"
#include "../../meta.h"


WorldRendererVulkanForward::WorldRendererVulkanForward(Renderer *parent, vulkan::Device *_device, Camera *cam) : WorldRendererVulkan("fw", parent, cam, RenderPathType::FORWARD) {
	device = _device;

	create_more();
}

static int cur_query_offset;

void WorldRendererVulkanForward::prepare(const RenderParams& params) {
	if (!cam)
		cam = cam_main;
	geo_renderer->cam = cam;
	shadow_renderer->cam = cam;


	static int pool_no = 0;
	pool_no = (pool_no + 1) % 16;
	cur_query_offset = pool_no * 8;
	device->reset_query_pool(cur_query_offset, 8);

	auto cb = command_buffer();

	cb->timestamp(cur_query_offset + 0);




#if 0
	static int _frame = 0;
	_frame ++;
	if (_frame > 10) {
		if (world.ego)
			render_into_cubemap(cb, cube_map.get(), world.ego->pos);
		_frame = 0;
	}
#endif

	cam->update_matrices(params.desired_aspect_ratio);

	prepare_lights(cam, geo_renderer->rvd_def);
	
	geo_renderer->prepare(params);

	if (shadow_index >= 0)
		shadow_renderer->render(cb, shadow_proj);

	cb->timestamp(cur_query_offset + 1);
}

void WorldRendererVulkanForward::draw(const RenderParams& params) {

	auto cb = command_buffer();
	auto rp = render_pass();

	auto &rvd = geo_renderer->rvd_def;

	geo_renderer->draw_skyboxes(cb, rp, params.desired_aspect_ratio, rvd);

	UBO ubo;
	ubo.p = cam->m_projection;
	ubo.v = cam->m_view;
	ubo.num_lights = lights.num;
	ubo.shadow_index = shadow_index;

	geo_renderer->draw_terrains(cb, rp, ubo, rvd);
	geo_renderer->draw_objects_opaque(cb, rp, ubo, rvd);
	geo_renderer->draw_objects_instanced(cb, rp, ubo, rvd);
	geo_renderer->draw_user_meshes(cb, rp, ubo, false, rvd);
	geo_renderer->draw_objects_transparent(cb, rp, ubo, rvd);

	geo_renderer->draw_particles(cb, rp, rvd);
	geo_renderer->draw_user_meshes(cb, rp, ubo, true, rvd);

	cb->timestamp(cur_query_offset + 2);
}

void WorldRendererVulkanForward::render_into_texture(CommandBuffer *cb, RenderPass *rp, FrameBuffer *fb, Camera *cam, RenderViewDataVK &rvd, const RenderParams& params) {
	auto cam0 = this->cam;
	this->cam = cam;

	geo_renderer->cam = cam;

	prepare_lights(cam, rvd);

	rp->clear_color[0] = background();

	cb->begin_render_pass(rp, fb);
	cb->set_viewport(rect(0, fb->width, 0, fb->height));


	geo_renderer->draw_skyboxes(cb, rp, params.desired_aspect_ratio, rvd);

	UBO ubo;
	ubo.p = cam->m_projection;
	ubo.v = cam->m_view;
	ubo.num_lights = lights.num;
	ubo.shadow_index = shadow_index;

	geo_renderer->draw_terrains(cb, rp, ubo, rvd);
	geo_renderer->draw_objects_opaque(cb, rp, ubo, rvd);
	geo_renderer->draw_objects_transparent(cb, rp, ubo, rvd);

	geo_renderer->draw_particles(cb, rp, rvd);

	cb->end_render_pass();

	this->cam = cam0;
}

#endif

