/*
 * WorldRendererVulkanForward.cpp
 *
 *  Created on: Nov 18, 2021
 *      Author: michi
 */

#include "WorldRendererVulkanRayTracing.h"
#ifdef USING_VULKAN
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

static const int MAX_RT_TRIAS = 65536;

WorldRendererVulkanRayTracing::WorldRendererVulkanRayTracing(Renderer *parent, vulkan::Device *_device) : WorldRendererVulkan("rt", parent, RenderPathType::FORWARD) {
	device = _device;

    offscreen_image = new vulkan::StorageTexture(width, height, 1, "rgba:i8");

    offscreen_image2 = new vulkan::Texture(width, height, "rgba:i8");

	buffer = new vulkan::UniformBuffer(sizeof(vec4) * MAX_RT_TRIAS);

    auto rt_pool = new vulkan::DescriptorPool("image:1,storage-buffer:1,buffer:1,sampler:1", 1);

    auto shader = ResourceManager::load_shader("vulkan/compute.shader");
    pipeline = new vulkan::ComputePipeline("[[image,buffer]]", shader);
    dset = rt_pool->create_set("image,buffer");
    dset->set_storage_image(0, offscreen_image);
    dset->set_buffer(1, buffer);
    dset->update();



	shader_out = ResourceManager::load_shader("vulkan/passthrough.shader");
	pipeline_out = new vulkan::GraphicsPipeline(shader_out.get(), parent->render_pass(), 0, "triangles", "3f,3f,2f");
	pipeline_out->set_culling(CullMode::NONE);
	pipeline_out->rebuild();
	dset_out = pool->create_set("sampler");

	dset_out->set_texture(0, offscreen_image2);
	dset_out->update();

	vb_2d = new VertexBuffer("3f,3f,2f");
	vb_2d->create_quad(rect::ID_SYM);
}

static int cur_query_offset;

void WorldRendererVulkanRayTracing::prepare() {
    #if 0
	static int pool_no = 0;
	pool_no = (pool_no + 1) % 16;
	cur_query_offset = pool_no * 8;
	device->reset_query_pool(cur_query_offset, 8);

	auto cb = command_buffer();

	cb->timestamp(cur_query_offset + 0);

	prepare_lights(cam_main, rvd_def);

	/*if (!shadow_cam) {
		shadow_entity = new Entity3D;
		shadow_cam = new Camera(rect::ID);
		shadow_entity->_add_component_external_(shadow_cam);
		shadow_entity->pos = cam->get_owner<Entity3D>()->pos;
		shadow_entity->ang = cam->get_owner<Entity3D>()->ang;
	}*/


	PerformanceMonitor::begin(ch_shadow);
	if (shadow_index >= 0) {
		render_shadow_map(cb, fb_shadow1.get(), 4, rvd_shadow1);
		render_shadow_map(cb, fb_shadow2.get(), 1, rvd_shadow2);
	}
	PerformanceMonitor::end(ch_shadow);
	cb->timestamp(cur_query_offset + 1);
    #endif


	Array<vec4> vertices;

	for (auto &s: world.sorted_opaque) {
		Model *m = s.model;
		auto& vb = m->mesh[0]->sub[s.mat_index].vertex_buffer->vertex_buffer;
		//msg_write(m->mesh[0]->sub[s.mat_index].vertex_buffer->index_buffer.size);
		auto p = (vulkan::Vertex1*)vb.map();
		for (int i=0; i<vb.size / sizeof(vulkan::Vertex1); i++)
			vertices.add(*(vec4*)&p[i].pos);
		if (vertices.num > 300)
			vertices.resize(300);
		msg_write(vertices.num);
		vb.unmap();
		break;
	}
	/*vertices.add({0,0.5,0,0});
	vertices.add({1,-0.5,0,0});
	vertices.add({-1,-0.5,0,0});*/


	buffer->update_array(vertices, 0);
	int num_trias = vertices.num / 3;




	auto cb = command_buffer();
	cb->image_barrier(offscreen_image,
        vulkan::AccessFlags::NONE, vulkan::AccessFlags::SHADER_WRITE_BIT,
        vulkan::ImageLayout::UNDEFINED, vulkan::ImageLayout::GENERAL);

    cb->set_bind_point(vulkan::PipelineBindPoint::COMPUTE);
    cb->bind_pipeline(pipeline);
    cb->bind_descriptor_set(0, dset);
	cb->push_constant(0, 4, &num_trias);
    cb->dispatch(width, height, 1);

    cb->set_bind_point(vulkan::PipelineBindPoint::GRAPHICS);

    /*cb->image_barrier(offscreen_image,
        vulkan::AccessFlags::SHADER_WRITE_BIT, vulkan::AccessFlags::SHADER_READ_BIT,
        vulkan::ImageLayout::GENERAL, vulkan::ImageLayout::SHADER_READ_ONLY_OPTIMAL);*/
	cb->copy_image(offscreen_image, offscreen_image2, {0,0,width,height,0,0});

    cb->image_barrier(offscreen_image,
        vulkan::AccessFlags::SHADER_WRITE_BIT, vulkan::AccessFlags::SHADER_READ_BIT,
        vulkan::ImageLayout::GENERAL, vulkan::ImageLayout::SHADER_READ_ONLY_OPTIMAL);

}

void WorldRendererVulkanRayTracing::draw() {

	/*auto cb = command_buffer();
	auto rp = render_pass();

	auto &rvd = rvd_def;

	draw_skyboxes(cb, rp, cam_main, (float)width / (float)height, rvd);

	UBO ubo;
	ubo.p = cam_main->m_projection;
	ubo.v = cam_main->m_view;
	ubo.num_lights = lights.num;
	ubo.shadow_index = shadow_index;

	draw_terrains(cb, rp, ubo, true, rvd);
	draw_objects_opaque(cb, rp, ubo, true, rvd);
	draw_objects_transparent(cb, rp, ubo, rvd);

	draw_particles(cb, rp, cam_main, rvd);

	cb->timestamp(cur_query_offset + 2);*/

	auto cb = command_buffer();

    //vb_2d->create_quad(rect::ID_SYM, rect::ID);//dynamicly_scaled_source());

	cb->bind_pipeline(pipeline_out);
	cb->bind_descriptor_set(0, dset_out);
	struct PCOut {
		mat4 p, m, v;
		float x[32];
	};
	PCOut pco = {mat4::ID, mat4::ID, mat4::ID, cam_main->exposure};
    pco.x[3] = 1; // scale_x
    pco.x[4] = 1;
	cb->push_constant(0, sizeof(mat4) * 3 + 5 * sizeof(float), &pco);
	cb->draw(vb_2d);
}

void WorldRendererVulkanRayTracing::render_into_texture(CommandBuffer *cb, RenderPass *rp, FrameBuffer *fb, Camera *cam, RenderViewDataVK &rvd) {

	/*prepare_lights(cam, rvd);

	rp->clear_color[0] = background();

	cb->begin_render_pass(rp, fb);
	cb->set_viewport(rect(0, fb->width, 0, fb->height));


	draw_skyboxes(cb, rp, cam, (float)fb->width / (float)fb->height, rvd);

	UBO ubo;
	ubo.p = cam->m_projection;
	ubo.v = cam->m_view;
	ubo.num_lights = lights.num;
	ubo.shadow_index = shadow_index;

	draw_terrains(cb, rp, ubo, true, rvd);
	draw_objects_opaque(cb, rp, ubo, true, rvd);
	draw_objects_transparent(cb, rp, ubo, rvd);

	draw_particles(cb, rp, cam, rvd);

	cb->end_render_pass();*/

}

#endif

