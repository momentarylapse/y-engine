/*
 * WorldRendererVulkanForward.cpp
 *
 *  Created on: Nov 18, 2021
 *      Author: michi
 */

#include "WorldRendererVulkanForward.h"
#ifdef USING_VULKAN
#include "../../graphics-impl.h"
#include "../base.h"
#include "../../lib/file/msg.h"

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
#include "../../world/Entity3D.h"
#include "../../world/Material.h"
#include "../../world/Model.h"
#include "../../world/Object.h" // meh
#include "../../world/Terrain.h"
#include "../../world/World.h"
#include "../../Config.h"
#include "../../meta.h"


WorldRendererVulkanForward::WorldRendererVulkanForward(Renderer *parent) : WorldRendererVulkan("fw", parent, RenderPathType::FORWARD) {
	shader_fx = ResourceManager::load_shader("vulkan/3d-fx.shader");
	pipeline_fx = new Pipeline(shader_fx.get(), render_pass(), 0, "triangles", "3f,4f,2f");
	pipeline_fx->set_blend(Alpha::SOURCE_ALPHA, Alpha::SOURCE_INV_ALPHA);
	pipeline_fx->set_z(true, false);
	pipeline_fx->set_culling(0);
	pipeline_fx->rebuild();
}

static int cur_query_offset;

void WorldRendererVulkanForward::prepare() {
	prepare_lights(cam);

	static int pool_no = 0;
	pool_no = (pool_no + 1) % 16;
	cur_query_offset = pool_no * 8;
	vulkan::default_device->reset_query_pool(cur_query_offset, 8);

	auto cb = command_buffer();

	cb->timestamp(cur_query_offset + 0);

	/*if (!shadow_cam) {
		shadow_entity = new Entity3D;
		shadow_cam = new Camera(rect::ID);
		shadow_entity->_add_component_external_(shadow_cam);
		shadow_entity->pos = cam->get_owner<Entity3D>()->pos;
		shadow_entity->ang = cam->get_owner<Entity3D>()->ang;
	}*/

	PerformanceMonitor::begin(ch_shadow);
	if (shadow_index >= 0) {
		render_shadow_map(cb, fb_shadow.get(), 4);
		render_shadow_map(cb, fb_shadow2.get(), 1);
	}
	PerformanceMonitor::end(ch_shadow);
	cb->timestamp(cur_query_offset + 1);
}

void WorldRendererVulkanForward::draw() {

	auto cb = command_buffer();
	auto rp = render_pass();

	draw_skyboxes(cb, cam);


	cam->update_matrices((float)width / (float)height);

	UBO ubo;
	ubo.p = cam->m_projection;
	ubo.v = cam->m_view;
	ubo.num_lights = lights.num;
	ubo.shadow_index = shadow_index;

	draw_terrains(cb, rp, ubo, true, rda_tr);
	draw_objects_opaque(cb, rp, ubo, true, rda_ob);
	draw_objects_transparent(cb, rp, ubo, rda_ob_trans);

	draw_particles(cb, rp);

	cb->timestamp(cur_query_offset + 2);
}

void WorldRendererVulkanForward::render_into_texture(FrameBuffer *fb, Camera *cam, const rect &target_area) {
}

void WorldRendererVulkanForward::render_shadow_map(CommandBuffer *cb, FrameBuffer *sfb, float scale) {

	cb->begin_render_pass(render_pass_shadow, sfb);
	cb->set_viewport(rect(0, shadow_resolution, 0, shadow_resolution));

	auto m = matrix::scale(scale, -scale, 1);
	//m = m * jitter(sfb->width*8, sfb->height*8, 1);

	//msg_write(shadow_proj.str());
	UBO ubo;
	ubo.p = m * shadow_proj;
	ubo.v = matrix::ID;
	ubo.num_lights = 0;
	ubo.shadow_index = -1;


	if (scale == 1) {
		draw_terrains(cb, render_pass_shadow, ubo, false, rda_tr_shadow);
		draw_objects_opaque(cb, render_pass_shadow, ubo, false, rda_ob_shadow);
	} else {
		draw_terrains(cb, render_pass_shadow, ubo, false, rda_tr_shadow2);
		draw_objects_opaque(cb, render_pass_shadow, ubo, false, rda_ob_shadow2);
	}

	cb->end_render_pass();
}

#endif

