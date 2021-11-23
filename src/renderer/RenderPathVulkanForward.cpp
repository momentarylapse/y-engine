/*
 * RenderPathVulkanForward.cpp
 *
 *  Created on: Nov 18, 2021
 *      Author: michi
 */

#include "RenderPathVulkanForward.h"
#ifdef USING_VULKAN
#include "../graphics-impl.h"
#include "base.h"
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


RenderPathVulkanForward::RenderPathVulkanForward(Renderer *parent) : RenderPathVulkan("fw", parent, RenderPathType::FORWARD) {


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
	}*/

	/*fb2 = new vulkan::FrameBuffer({
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
	ResourceManager::load_shader("module-vertex-instanced.shader");*/

	/*shader_depth = ResourceManager::load_shader("forward/depth.shader");
	shader_fx = ResourceManager::load_shader("forward/3d-fx.shader");
	shader_resolve_multisample = ResourceManager::load_shader("forward/resolve-multisample.shader");*/
}

void RenderPathVulkanForward::prepare() {
	prepare_lights(cam);
}

void RenderPathVulkanForward::draw() {

	auto cb = current_command_buffer();

	draw_world(cb, true);

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

*/
}

void RenderPathVulkanForward::render_into_texture(FrameBuffer *fb, Camera *cam, const rect &target_area) {
}

void RenderPathVulkanForward::draw_world(CommandBuffer *cb, bool allow_material) {

	draw_skyboxes(cb, cam);
	draw_terrains(cb, allow_material);
	draw_objects_opaque(cb, allow_material);

	/*draw_terrains(allow_material);
	draw_objects_instanced(allow_material);
	draw_objects_opaque(allow_material);
	if (allow_material)
		draw_objects_transparent(allow_material, type);*/
}

void RenderPathVulkanForward::render_shadow_map(FrameBuffer *sfb, float scale) {
}

#endif

