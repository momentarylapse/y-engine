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

	if (device->has_rtx())
		mode = Mode::RTX;
	else if (device->has_compute())
		mode = Mode::COMPUTE;
	else
		throw Exception("");

    offscreen_image = new vulkan::StorageTexture(width, height, 1, "rgba:f16");
    offscreen_image2 = new vulkan::Texture(width, height, "rgba:f16");


	if (mode == Mode::RTX) {
		msg_error("RTX!!!");

		auto shader_gen = ResourceManager::load_shader("vulkan/gen.shader");
		auto shader1 = ResourceManager::load_shader("vulkan/group1.shader");
		auto shader2 = ResourceManager::load_shader("vulkan/group2.shader");
		pipeline_rtx = new vulkan::RayPipeline("[[acceleration-structure,image,buffer,buffer]]", {shader_gen, shader1, shader2}, 2);
		pipeline_rtx->create_sbt();

	} else if (mode == Mode::COMPUTE) {
		buffer_vertices = new vulkan::UniformBuffer(sizeof(vec4) * MAX_RT_TRIAS);
		buffer_materials = new vulkan::UniformBuffer(sizeof(vec4) * MAX_RT_TRIAS * 2);

		auto rt_pool = new vulkan::DescriptorPool("image:1,storage-buffer:1,buffer:8,sampler:1", 1);

		auto shader = ResourceManager::load_shader("vulkan/compute.shader");
		pipeline = new vulkan::ComputePipeline("[[image,buffer,buffer,buffer]]", shader);
		dset = rt_pool->create_set("image,buffer,buffer,buffer");
		dset->set_storage_image(0, offscreen_image);
		dset->set_buffer(1, buffer_vertices);
		dset->set_buffer(2, buffer_materials);
		dset->set_buffer(3, rvd_def.ubo_light);
		dset->update();
	}



	shader_out = ResourceManager::load_shader("vulkan/passthrough.shader");
	pipeline_out = new vulkan::GraphicsPipeline(shader_out.get(), parent->render_pass(), 0, "triangles", "3f,3f,2f");
	pipeline_out->set_culling(CullMode::NONE);
	pipeline_out->rebuild();
	dset_out = pool->create_set("sampler");

	dset_out->set_texture(0, offscreen_image2);
	dset_out->update();

	vb_2d = new VertexBuffer("3f,3f,2f");
	vb_2d->create_quad(rect::ID_SYM);


	dummy_cam_entity = new Entity;
	dummy_cam = new Camera(rect::ID);
	dummy_cam_entity->components.add(dummy_cam);
	dummy_cam->owner = dummy_cam_entity;
}

static int cur_query_offset;

void WorldRendererVulkanRayTracing::prepare() {
	
	prepare_lights(dummy_cam, rvd_def);

	auto cb = command_buffer();

	if (mode == Mode::RTX) {
		for (auto &s: world.sorted_opaque) {
			Model *m = s.model;
			m->update_matrix();

			m->mesh[0]->sub[s.mat_index].vertex_buffer
			
			/*for (int i=0; i<m->mesh[0]->sub[s.mat_index].triangle_index.num/3; i++) {
				materials.add(s.material->albedo.with_alpha(s.material->roughness));
				materials.add(s.material->emission.with_alpha(s.material->metal));
			}*/
			break;
		}
		
	} else if (mode == Mode::COMPUTE) {

		Array<vec4> vertices;
		Array<color> materials;

		for (auto &s: world.sorted_opaque) {
			Model *m = s.model;
			m->update_matrix();
			for (int t: m->mesh[0]->sub[s.mat_index].triangle_index)
				vertices.add(vec4(m->_matrix * m->mesh[0]->vertex[t], 0));
			for (int i=0; i<m->mesh[0]->sub[s.mat_index].triangle_index.num/3; i++) {
				materials.add(s.material->albedo.with_alpha(s.material->roughness));
				materials.add(s.material->emission.with_alpha(s.material->metal));
			}

			if (vertices.num > 1000) {
				vertices.resize(1000);
				break;
			}
		}


		buffer_vertices->update_array(vertices, 0);
		buffer_materials->update_array(materials, 0);
		int num_trias = vertices.num / 3;


		cb->image_barrier(offscreen_image,
			vulkan::AccessFlags::NONE, vulkan::AccessFlags::SHADER_WRITE_BIT,
			vulkan::ImageLayout::UNDEFINED, vulkan::ImageLayout::GENERAL);

		cb->set_bind_point(vulkan::PipelineBindPoint::COMPUTE);
		cb->bind_pipeline(pipeline);
		cb->bind_descriptor_set(0, dset);
		struct PushConst {
			mat4 iview;
			color background;
			int num_trias;
			int num_lights;
		} pc;
		pc.iview = cam_main->view_matrix().inverse();
		pc.background = background();
		pc.num_trias = num_trias;
		pc.num_lights = lights.num;
		cb->push_constant(0, sizeof(pc), &pc);
		cb->dispatch(width, height, 1);
	}

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
}

#endif

