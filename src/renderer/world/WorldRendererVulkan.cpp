/*
 * WorldRendererVulkan.cpp
 *
 *  Created on: Nov 18, 2021
 *      Author: michi
 */

#include "WorldRendererVulkan.h"
#ifdef USING_VULKAN
#include "pass/ShadowRendererVulkan.h"
#include "../base.h"
#include "../helper/PipelineManager.h"
#include "../../graphics-impl.h"
#include "../../lib/image/image.h"
#include "../../lib/math/vec3.h"
#include "../../lib/math/complex.h"
#include "../../lib/math/rect.h"
#include "../../lib/os/msg.h"
#include "../../helper/PerformanceMonitor.h"
#include "../../helper/ResourceManager.h"
#include "../../plugins/PluginManager.h"
#include "../../fx/Particle.h"
#include "../../fx/Beam.h"
#include "../../fx/ParticleManager.h"
#include "../../gui/gui.h"
#include "../../gui/Picture.h"
#include "../../world/Camera.h"
#include "../../world/Material.h"
#include "../../world/Model.h"
#include "../../world/Object.h" // meh
#include "../../world/Terrain.h"
#include "../../world/World.h"
#include "../../world/Light.h"
#include "../../world/components/Animator.h"
#include "../../world/components/UserMesh.h"
#include "../../world/components/MultiInstance.h"
#include "../../y/Entity.h"
#include "../../y/ComponentManager.h"
#include "../../Config.h"
#include "../../meta.h"


const int CUBE_SIZE = 128;



WorldRendererVulkan::WorldRendererVulkan(const string &name, Renderer *parent, Camera *cam, RenderPathType _type) : WorldRenderer(name, parent, cam) {
	type = _type;

	vb_2d = nullptr;


	cube_map = new CubeMap(CUBE_SIZE, "rgba:i8");
	if (false) {
		Image im;
		im.create(CUBE_SIZE, CUBE_SIZE, Red);
		cube_map->write_side(0, im);
		im.create(CUBE_SIZE, CUBE_SIZE, color(1, 1,0.5f,0));
		cube_map->write_side(1, im);
		im.create(CUBE_SIZE, CUBE_SIZE, color(1, 1,0,1));
		cube_map->write_side(2, im);
	}

	depth_cube = new DepthBuffer(CUBE_SIZE, CUBE_SIZE, "d:f32", true);

	render_pass_cube = new vulkan::RenderPass({cube_map.get(), depth_cube.get()}, "clear");
	fb_cube = new vulkan::FrameBuffer(render_pass_cube, {cube_map.get(), depth_cube.get()});



	vb_2d = new VertexBuffer("3f,3f,2f");
	vb_2d->create_quad(rect::ID_SYM);




	resource_manager->default_shader = "default.shader";
	/*if (config.get_str("renderer.shader-quality", "pbr") == "pbr") {
		resource_manager->load_shader_module("module-lighting-pbr.shader");
		resource_manager->load_shader_module("forward/module-surface-pbr.shader");
	} else {
		resource_manager->load_shader_module("forward/module-surface.shader");
	}
	resource_manager->load_shader_module("module-vertex-default.shader");
	resource_manager->load_shader_module("module-vertex-animated.shader");
	resource_manager->load_shader_module("module-vertex-instanced.shader");*/
	resource_manager->load_shader_module("vulkan/module-surface-dummy.shader");
	resource_manager->load_shader_module("module-vertex-default.shader");
	resource_manager->load_shader_module("module-vertex-animated.shader");
	resource_manager->load_shader_module("module-vertex-instanced.shader");
	resource_manager->load_shader_module("module-vertex-fx.shader");
	resource_manager->load_shader_module("module-vertex-points.shader");
	resource_manager->load_shader_module("module-vertex-lines.shader");
	resource_manager->load_shader_module("module-geometry-points.shader");
	resource_manager->load_shader_module("module-geometry-lines.shader");
}

void WorldRendererVulkan::create_more() {
	shadow_renderer = new ShadowRendererVulkan(this);
	fb_shadow1 = shadow_renderer->fb[0];
	fb_shadow2 = shadow_renderer->fb[1];

	geo_renderer = new GeometryRendererVulkan(type, this);
	geo_renderer->cube_map = cube_map;
	geo_renderer->material_shadow = shadow_renderer->material;
	geo_renderer->fb_shadow1 = shadow_renderer->fb[0];
	geo_renderer->fb_shadow2 = shadow_renderer->fb[1];

}

WorldRendererVulkan::~WorldRendererVulkan() {
}


void WorldRendererVulkan::render_into_cubemap(CommandBuffer *cb, CubeMap *cube, const vec3 &pos) {
	if (!fb_cube)
		fb_cube = new FrameBuffer(render_pass_cube, {depth_cube.get()});
	Entity o(pos, quaternion::ID);
	Camera cam;
	cam.owner = &o;
	cam.fov = pi/2;
	for (int i=0; i<6; i++) {
		try {
			fb_cube->update_x(render_pass_cube, {cube, depth_cube.get()}, i);
		} catch(Exception &e) {
			msg_error(e.message());
			return;
		}
		if (i == 0)
			o.ang = quaternion::rotation(vec3(0,pi/2,0));
		if (i == 1)
			o.ang = quaternion::rotation(vec3(0,-pi/2,0));
		if (i == 2)
			o.ang = quaternion::rotation(vec3(-pi/2,pi,pi));
		if (i == 3)
			o.ang = quaternion::rotation(vec3(pi/2,pi,pi));
		if (i == 4)
			o.ang = quaternion::rotation(vec3(0,0,0));
		if (i == 5)
			o.ang = quaternion::rotation(vec3(0,pi,0));
		render_into_texture(cb, render_pass_cube, fb_cube.get(), &cam, rvd_cube[i], RenderParams::INTO_TEXTURE);
	}
	cam.owner = nullptr;
}


void WorldRendererVulkan::prepare_lights(Camera *cam, RenderViewDataVK &rvd) {
	PerformanceMonitor::begin(ch_prepare_lights);

	lights.clear();
	for (auto *l: world.lights) {
		if (!l->enabled)
			continue;

		l->update(cam, shadow_box_size, geo_renderer->using_view_space);

		if (l->allow_shadow) {
			shadow_index = lights.num;
			shadow_proj = l->shadow_projection;
		}
		lights.add(l->light);
	}
	rvd.ubo_light->update_part(&lights[0], 0, lights.num * sizeof(lights[0]));
	PerformanceMonitor::end(ch_prepare_lights);
}

#endif



