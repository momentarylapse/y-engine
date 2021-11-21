/*
 * RenderPathVulkanForward.cpp
 *
 *  Created on: Nov 18, 2021
 *      Author: michi
 */

#include "RenderPathVulkanForward.h"
#ifdef USING_VULKAN
#include "WindowRendererVulkan.h"
#include "../graphics-impl.h"
#include "../lib/image/image.h"
#include "../lib/file/msg.h"

#include "../helper/PerformanceMonitor.h"
#include "../helper/ResourceManager.h"
#include "../helper/Scheduler.h"
#include "../plugins/PluginManager.h"
#include "../gui/gui.h"
#include "../gui/Node.h"
#include "../gui/Picture.h"
#include "../gui/Text.h"
#include "../fx/Particle.h"
#include "../fx/Beam.h"
#include "../fx/ParticleManager.h"
#include "../world/Camera.h"
#include "../world/Light.h"
#include "../world/Entity3D.h"
#include "../world/Material.h"
#include "../world/Model.h"
#include "../world/Object.h" // meh
#include "../world/Terrain.h"
#include "../world/World.h"
#include "../Config.h"
#include "../meta.h"


RenderPathVulkanForward::RenderPathVulkanForward(WindowRendererVulkan *r) : RenderPathVulkan(r, RenderPathType::FORWARD) {

	/*depth_buffer = new vulkan::DepthBuffer(width, height, "d24s8");
	if (config.antialiasing_method == AntialiasingMethod::MSAA) {
		fb_main = new vulkan::FrameBuffer({
			new vulkan::TextureMultiSample(width, height, 4, "rgba:f16"),
			//depth_buffer});
			new vulkan::RenderBuffer(width, height, 4, "d24s8")});
	} else {
		fb_main = new vulkan::FrameBuffer({
			new vulkan::Texture(width, height, "rgba:f16"),
			depth_buffer});
			//new vulkan::RenderBuffer(width, height, "d24s8)});
	}
	fb_small1 = new vulkan::FrameBuffer({
		new vulkan::Texture(width/2, height/2, "rgba:f16")});
	fb_small2 = new vulkan::FrameBuffer({
		new vulkan::Texture(width/2, height/2, "rgba:f16")});
	fb2 = new vulkan::FrameBuffer({
		new vulkan::Texture(width, height, "rgba:f16")});
	fb3 = new vulkan::FrameBuffer({
		new vulkan::Texture(width, height, "rgba:f16")});
	fb_shadow = new vulkan::FrameBuffer({
		new vulkan::DepthBuffer(shadow_resolution, shadow_resolution, "d24s8")});
	fb_shadow2 = new vulkan::FrameBuffer({
		new vulkan::DepthBuffer(shadow_resolution, shadow_resolution, "d24s8")});

	if (fb_main->color_attachments[0]->type != nix::Texture::Type::MULTISAMPLE)
		fb_main->color_attachments[0]->set_options("wrap=clamp");
	fb_small1->color_attachments[0]->set_options("wrap=clamp");
	fb_small2->color_attachments[0]->set_options("wrap=clamp");
	fb2->color_attachments[0]->set_options("wrap=clamp");
	fb3->color_attachments[0]->set_options("wrap=clamp");

	ResourceManager::default_shader = "default.shader";
	if (config.get_str("renderer.shader-quality", "") == "pbr") {
		ResourceManager::load_shader("module-lighting-pbr.shader");
		ResourceManager::load_shader("forward/module-surface-pbr.shader");
	} else {
		ResourceManager::load_shader("forward/module-surface.shader");
	}
	ResourceManager::load_shader("module-vertex-default.shader");
	ResourceManager::load_shader("module-vertex-animated.shader");
	ResourceManager::load_shader("module-vertex-instanced.shader");

	shader_blur = ResourceManager::load_shader("forward/blur.shader");
	shader_depth = ResourceManager::load_shader("forward/depth.shader");
	shader_out = ResourceManager::load_shader("forward/hdr.shader");
	shader_fx = ResourceManager::load_shader("forward/3d-fx.shader");
	shader_2d = ResourceManager::load_shader("forward/2d.shader");
	shader_resolve_multisample = ResourceManager::load_shader("forward/resolve-multisample.shader");


//	if (nix::total_mem() > 0) {
//		msg_write(format("VRAM: %d mb  of  %d mb available", nix::available_mem() / 1024, nix::total_mem() / 1024));
//	}*/
}

void RenderPathVulkanForward::draw() {


	auto cb = renderer->current_command_buffer();
	auto rp = renderer->default_render_pass();
	auto fb = renderer->current_frame_buffer();

	prepare_gui(fb);
	prepare_lights(cam);


	cb->begin();

	cb->set_viewport(renderer->area());

	rp->clear_color[0] = world.background;
	cb->begin_render_pass(rp, fb);

	draw_skyboxes(cb, cam);
	draw_objects_opaque(cb, true);
	draw_terrains(cb, true);

	/*cb->bind_pipeline(pipeline);
	cb->bind_descriptor_set(0, dset);
	float x = 0;
	cb->push_constant(0,4,&x);

	cb->draw(vb);*/

	draw_gui(cb);

	cb->end_render_pass();
	cb->end();


	/*PerformanceMonitor::begin(ch_render);

	static int _frame = 0;
	_frame ++;
	if (_frame > 10) {
		if (world.ego)
			render_into_cubemap(depth_cube.get(), cube_map.get(), world.ego->pos);
		_frame = 0;
	}


	prepare_instanced_matrices();

	prepare_lights(cam);

	PerformanceMonitor::begin(ch_shadow);
	if (shadow_index >= 0) {
		render_shadow_map(fb_shadow.get(), 4);
		render_shadow_map(fb_shadow2.get(), 1);
	}
	PerformanceMonitor::end(ch_shadow);

	render_into_texture(fb_main.get(), cam, dynamic_fb_area());

	auto source = fb_main.get();
	if (config.antialiasing_method == AntialiasingMethod::MSAA)
		source = resolve_multisampling(source);

	source = do_post_processing(source);


	nix::bind_frame_buffer(nix::FrameBuffer::DEFAULT);
	render_out(source, fb_small2->color_attachments[0].get());

	draw_gui(source);
	PerformanceMonitor::end(ch_render);*/
}

void RenderPathVulkanForward::render_into_texture(FrameBuffer *fb, Camera *cam, const rect &target_area) {
}

void RenderPathVulkanForward::draw_world(bool allow_material) {
	/*draw_terrains(allow_material);
	draw_objects_instanced(allow_material);
	draw_objects_opaque(allow_material);
	if (allow_material)
		draw_objects_transparent(allow_material, type);*/
}

void RenderPathVulkanForward::render_shadow_map(FrameBuffer *sfb, float scale) {
}

#endif

