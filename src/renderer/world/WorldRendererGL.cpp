/*
 * WorldRendererGL.cpp
 *
 *  Created on: 07.08.2020
 *      Author: michi
 */

#include "WorldRendererGL.h"

#include <GLFW/glfw3.h>

#ifdef USING_OPENGL
#include "geometry/GeometryRendererGL.h"
#include "pass/ShadowRendererGL.h"
#include "../base.h"
#include "../../graphics-impl.h"
#include "../../world/World.h"
#include "../../world/Light.h"
#include "../../helper/PerformanceMonitor.h"


namespace nix {
	void resolve_multisampling(FrameBuffer *target, FrameBuffer *source);
}
void apply_shader_data(Shader *s, const Any &shader_data);

const int CUBE_SIZE = 128;


WorldRendererGL::WorldRendererGL(const string &name, Renderer *parent, RenderPathType _type) : WorldRenderer(name, parent) {
	type = _type;

	depth_cube = new nix::DepthBuffer(CUBE_SIZE, CUBE_SIZE, "d24s8");
	fb_cube = nullptr;
	cube_map = new nix::CubeMap(CUBE_SIZE, "rgba:i8");

	ubo_light = new nix::UniformBuffer();
}

void WorldRendererGL::create_more() {
	geo_renderer = new GeometryRendererGL(type, this);

	shadow_renderer = new ShadowRendererGL(this);
	fb_shadow1 = shadow_renderer->fb[0];
	fb_shadow2 = shadow_renderer->fb[1];
	material_shadow = shadow_renderer->material;
	geo_renderer->material_shadow = shadow_renderer->material;
}

void WorldRendererGL::prepare_lights() {
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
	ubo_light->update_array(lights);
	geo_renderer->ubo_light = ubo_light;
	geo_renderer->num_lights = lights.num;
	geo_renderer->shadow_index = shadow_index;
	geo_renderer->shadow_proj = shadow_proj;
	PerformanceMonitor::end(ch_prepare_lights);
}

void WorldRendererGL::render_into_cubemap(DepthBuffer *depth, CubeMap *cube, const vec3 &pos) {
	#if 0
	if (!fb_cube)
		fb_cube = new nix::FrameBuffer({depth});
	Entity o(pos, quaternion::ID);
	Camera cam(rect::ID);
	cam.owner = &o;
	cam.fov = pi/2;
	for (int i=0; i<6; i++) {
		try {
			fb_cube->update_x({cube, depth}, i);
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
		prepare_lights(&cam);
		render_into_texture(fb_cube.get(), &cam);
	}
	cam.owner = nullptr;
	#endif
}




#endif
