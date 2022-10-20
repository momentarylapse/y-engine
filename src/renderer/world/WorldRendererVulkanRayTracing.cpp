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
#include "../../lib/base/iter.h"
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
static const int MAX_RT_MESHES = 1024;

WorldRendererVulkanRayTracing::WorldRendererVulkanRayTracing(Renderer *parent, vulkan::Device *_device) : WorldRendererVulkan("rt", parent, RenderPathType::FORWARD) {
	device = _device;

	if (device->has_rtx() and config.allow_rtx)
		mode = Mode::RTX;
	else if (device->has_compute())
		mode = Mode::COMPUTE;
	else
		throw Exception("neither RTX nor compute shader support");

	offscreen_image = new vulkan::StorageTexture(width, height, 1, "rgba:f16");
	offscreen_image2 = new vulkan::Texture(width, height, "rgba:f16");


	if (mode == Mode::RTX) {
		msg_error("RTX!!!");
		rtx.pool = new vulkan::DescriptorPool("acceleration-structure:1,image:1,storage-buffer:1,buffer:1024,sampler:1024", 1024);


		rtx.buffer_cam = new vulkan::UniformBuffer(sizeof(PushConst));
		rtx.buffer_vertices = new vulkan::UniformBuffer(sizeof(vec4) * 5 * MAX_RT_TRIAS * 3);


		rtx.dset = rtx.pool->create_set("acceleration-structure,image,buffer,buffer,buffer");
		rtx.dset->set_storage_image(1, offscreen_image);
		rtx.dset->set_buffer(2, rtx.buffer_cam);
		rtx.dset->set_buffer(4, rvd_def.ubo_light);

		auto shader_gen = ResourceManager::load_shader("vulkan/gen.shader");
		auto shader1 = ResourceManager::load_shader("vulkan/group1.shader");
		auto shader2 = ResourceManager::load_shader("vulkan/group2.shader");
		rtx.pipeline = new vulkan::RayPipeline("[[acceleration-structure,image,buffer,buffer,buffer]]", {shader_gen, shader1, shader2}, 2);
		rtx.pipeline->create_sbt();


	} else if (mode == Mode::COMPUTE) {
		compute.buffer_meshes = new vulkan::UniformBuffer(sizeof(MeshDescription) * MAX_RT_MESHES);

		compute.pool = new vulkan::DescriptorPool("image:1,storage-buffer:1,buffer:8,sampler:1", 1);

		auto shader = ResourceManager::load_shader("vulkan/compute.shader");
		compute.pipeline = new vulkan::ComputePipeline("[[image,buffer,buffer]]", shader);
		compute.dset = compute.pool->create_set("image,buffer,buffer");
		compute.dset->set_storage_image(0, offscreen_image);
		compute.dset->set_buffer(1, compute.buffer_meshes);
		compute.dset->set_buffer(2, rvd_def.ubo_light);
		compute.dset->update();
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

	pc.iview = cam_main->view_matrix().inverse();
	pc.background = background();
	pc.num_lights = lights.num;

	auto cb = command_buffer();

	cb->image_barrier(offscreen_image,
		vulkan::AccessFlags::NONE, vulkan::AccessFlags::SHADER_WRITE_BIT,
		vulkan::ImageLayout::UNDEFINED, vulkan::ImageLayout::GENERAL);

	if (mode == Mode::RTX) {

		Array<mat4> matrices;

		if (rtx.tlas) {
			// update
			for (auto &s: world.sorted_opaque) {
				Model *m = s.model;
				m->update_matrix();
				matrices.add(m->owner->get_matrix().transpose());
			}
			rtx.tlas->update_top(rtx.blas, matrices);

		} else {

			auto c2v4 = [] (const color &c) {
				return *(const vec4*)&c;
			};

			auto v32v4 = [] (const vec3 &v, float w) {
				return vec4(v.x, v.y, v.z, w);
			};

			Array<vec4> vertices;

			for (auto &s: world.sorted_opaque) {
				Model *m = s.model;
				m->update_matrix();
				auto vb = m->mesh[0]->sub[s.mat_index].vertex_buffer;
				if (!vb->is_indexed()) {
					Array<int> index;
					for (int i=0; i<m->mesh[0]->sub[s.mat_index].num_triangles*3; i++)
						index.add(i);
					vb->update_index(index);
				}
				rtx.blas.add(vulkan::AccelerationStructure::create_bottom(device, vb));
				matrices.add(m->owner->get_matrix().transpose());
				
				for (int i=0; i<m->mesh[0]->sub[s.mat_index].triangle_index.num/3; i++) {
					vertices.add(v32v4(m->mesh[0]->vertex[m->mesh[0]->sub[s.mat_index].triangle_index[i*3]], 0));
					vertices.add(v32v4(m->mesh[0]->vertex[m->mesh[0]->sub[s.mat_index].triangle_index[i*3+1]], 0));
					vertices.add(v32v4(m->mesh[0]->vertex[m->mesh[0]->sub[s.mat_index].triangle_index[i*3+2]], 0));
					vertices.add(c2v4(s.material->albedo.with_alpha(s.material->roughness)));
					vertices.add(c2v4(s.material->emission.with_alpha(s.material->metal)));
				}
			}
			msg_write(vertices.num);
			rtx.tlas = vulkan::AccelerationStructure::create_top(device, rtx.blas, matrices);

			rtx.buffer_vertices->update_array(vertices, 0);
		}

		rtx.buffer_cam->update(&pc);
		rtx.dset->set_acceleration_structure(0, rtx.tlas);
		rtx.dset->set_buffer(3, rtx.buffer_vertices);
		rtx.dset->update();
		
		cb->set_bind_point(vulkan::PipelineBindPoint::RAY_TRACING);
		cb->bind_pipeline(rtx.pipeline);

		cb->bind_descriptor_set(0, rtx.dset);
		//cb->push_constant(0, sizeof(pc), &pc);

		cb->trace_rays(width, height, 1);
		
	} else if (mode == Mode::COMPUTE) {

		Array<MeshDescription> meshes;

		for (auto &s: world.sorted_opaque) {
			Model *m = s.model;
			m->update_matrix();

			MeshDescription md;
			md.matrix = m->_matrix;
			md.num_triangles = m->mesh[0]->sub[s.mat_index].triangle_index.num / 3;
			md.albedo = s.material->albedo.with_alpha(s.material->roughness);
			md.emission = s.material->emission.with_alpha(s.material->metal);
			md.address_vertices = m->mesh[0]->sub[s.mat_index].vertex_buffer->vertex_buffer.get_device_address();
			//md.address_indices = m->mesh[0]->sub[s.mat_index].vertex_buffer->index_buffer.get_device_address();
			meshes.add(md);
			if (meshes.num > 10)
				break;
		}

		pc.num_trias = 0;
		pc.num_meshes = meshes.num;

		compute.buffer_meshes->update_array(meshes, 0);


		cb->set_bind_point(vulkan::PipelineBindPoint::COMPUTE);
		cb->bind_pipeline(compute.pipeline);
		cb->bind_descriptor_set(0, compute.dset);
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

