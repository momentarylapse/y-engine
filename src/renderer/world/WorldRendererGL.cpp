/*
 * WorldRendererGL.cpp
 *
 *  Created on: 07.08.2020
 *      Author: michi
 */

#include "WorldRendererGL.h"

#include <renderer/helper/CubeMapSource.h>

#ifdef USING_OPENGL
#include "geometry/GeometryRendererGL.h"
#include "pass/ShadowRenderer.h"
#include "../base.h"
#include <graphics-impl.h>
#include <world/World.h>
#include <world/Light.h>
#include <world/Camera.h>
#include <helper/PerformanceMonitor.h>
#include <y/ComponentManager.h>
#include <lib/os/msg.h>


namespace nix {
	void resolve_multisampling(FrameBuffer *target, FrameBuffer *source);
}
void apply_shader_data(Shader *s, const Any &shader_data);


WorldRendererGL::WorldRendererGL(const string &name, Camera *cam, RenderPathType _type) :
		WorldRenderer(name, cam) {
	type = _type;

	// not sure this is a good idea...
	auto e = new Entity;
	cube_map_source = new CubeMapSource;
	cube_map_source->owner = e;
	cube_map_source->cube_map = new CubeMap(cube_map_source->resolution, "rgba:i8");

	scene_view.cube_map = cube_map_source->cube_map;
}

void WorldRendererGL::create_more() {
	shadow_renderer = new ShadowRenderer();
	scene_view.fb_shadow1 = shadow_renderer->cascades[0].fb;
	scene_view.fb_shadow2 = shadow_renderer->cascades[1].fb;
	add_child(shadow_renderer.get());

	geo_renderer = new GeometryRenderer(type, scene_view);
	add_child(geo_renderer.get());
}

void WorldRendererGL::prepare_lights(Camera *cam, RenderViewData &rvd) {
	PerformanceMonitor::begin(ch_prepare_lights);
	scene_view.prepare_lights(shadow_box_size, rvd.ubo_light.get());
	PerformanceMonitor::end(ch_prepare_lights);
}

void WorldRendererGL::render_into_cubemap(CubeMapSource& source) {
	if (!source.depth_buffer)
		source.depth_buffer = new nix::DepthBuffer(source.resolution, source.resolution, "d24s8");
	if (!source.cube_map)
		source.cube_map = new CubeMap(source.resolution, "rgba:i8");
	if (!source.frame_buffer[0])
		for (int i=0; i<6; i++) {
			source.frame_buffer[i] = new nix::FrameBuffer();
			try {
				source.frame_buffer[i]->update_x({source.cube_map.get(), source.depth_buffer.get()}, i);
			} catch(Exception &e) {
				msg_error(e.message());
				return;
			}
		}
	Entity o(source.owner->pos, quaternion::ID);
	Camera cam;
	cam.min_depth = source.min_depth;
	cam.owner = &o;
	cam.fov = pi/2;
	for (int i=0; i<6; i++) {
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
		//prepare_lights(&cam);
		render_into_texture(source.frame_buffer[i].get(), &cam, source.rvd[i]);
	}
	cam.owner = nullptr;
}




#endif
