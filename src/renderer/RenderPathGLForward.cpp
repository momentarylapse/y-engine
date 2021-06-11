/*
 * RenderPathGLForward.cpp
 *
 *  Created on: Jun 2, 2021
 *      Author: michi
 */

#include <GLFW/glfw3.h>
#include "RenderPathGLForward.h"
#include "../lib/nix/nix.h"
#include "../lib/image/image.h"

#include "../helper/PerformanceMonitor.h"
#include "../helper/ResourceManager.h"
#include "../plugins/PluginManager.h"
#include "../gui/gui.h"
#include "../gui/Node.h"
#include "../gui/Picture.h"
#include "../gui/Text.h"
#include "../fx/Light.h"
#include "../fx/Particle.h"
#include "../fx/Beam.h"
#include "../fx/ParticleManager.h"
#include "../world/Camera.h"
#include "../world/Material.h"
#include "../world/Model.h"
#include "../world/Object.h" // meh
#include "../world/Terrain.h"
#include "../world/World.h"
#include "../Config.h"
#include "../meta.h"

// https://learnopengl.com/Advanced-OpenGL/Anti-Aliasing

namespace nix {
	int total_mem();
	int available_mem();
}
matrix jitter(float w, float h, int uid);
void break_point();

nix::UniformBuffer *ubo_multi_matrix = nullptr;


RenderPathGLForward::RenderPathGLForward(GLFWwindow* win, int w, int h, PerformanceMonitor *pm) : RenderPathGL(win, w, h, pm) {
	depth_buffer = new nix::DepthBuffer(width, height);
	if (config.antialiasing_method == AntialiasingMethod::MSAA) {
		depth_buffer = new nix::DepthBuffer(width, height);
		fb_main = new nix::FrameBuffer({
			new nix::TextureMultiSample(width, height, 4, "rgba:f16"),
			//depth_buffer});
			new nix::RenderBuffer(width, height, 4)});
	} else {
		fb_main = new nix::FrameBuffer({
			new nix::Texture(width, height, "rgba:f16"),
			depth_buffer});
			//new nix::RenderBuffer(width, height)});
	}
	fb_small1 = new nix::FrameBuffer({
		new nix::Texture(width/2, height/2, "rgba:f16")});
	fb_small2 = new nix::FrameBuffer({
		new nix::Texture(width/2, height/2, "rgba:f16")});
	fb2 = new nix::FrameBuffer({
		new nix::Texture(width, height, "rgba:f16")});
	fb3 = new nix::FrameBuffer({
		new nix::Texture(width, height, "rgba:f16")});
	fb_shadow = new nix::FrameBuffer({
		new nix::DepthBuffer(shadow_resolution, shadow_resolution)});
	fb_shadow2 = new nix::FrameBuffer({
		new nix::DepthBuffer(shadow_resolution, shadow_resolution)});

	fb_main->color_attachments[0]->set_options("wrap=clamp");
	fb_small1->color_attachments[0]->set_options("wrap=clamp");
	fb_small2->color_attachments[0]->set_options("wrap=clamp");
	fb2->color_attachments[0]->set_options("wrap=clamp");
	fb3->color_attachments[0]->set_options("wrap=clamp");

	if (config.get_str("renderer.shader-quality", "") == "pbr")
		ResourceManager::load_shader(":E:/forward/module-surface-pbr.shader");
	else
		ResourceManager::load_shader(":E:/forward/module-surface.shader");

	shader_blur = ResourceManager::load_shader(":E:/forward/blur.shader");
	shader_depth = ResourceManager::load_shader(":E:/forward/depth.shader");
	shader_out = ResourceManager::load_shader(":E:/forward/hdr.shader");
	//shader_3d = ResourceManager::load_shader(":E:/forward/3d-new.shader");
	shader_3d = ResourceManager::load_shader(":E:/default.shader");
	shader_3d_multi = ResourceManager::load_shader(":E:/default-multi.shader");
	if (!shader_3d_multi->link_uniform_block("Multi", 5))
		msg_error("Multi not found...");
	shader_fx = ResourceManager::load_shader(":E:/forward/3d-fx.shader");
	//nix::default_shader_3d = shader_3d;
	shader_shadow = ResourceManager::load_shader(":E:/forward/3d-shadow.shader");

	shader_2d = ResourceManager::load_shader(":E:/forward/2d.shader");
	shader_resolve_multisample = ResourceManager::load_shader(":E:/forward/resolve-multisample.shader");

	ubo_multi_matrix = new nix::UniformBuffer();


	if (nix::total_mem() > 0) {
		msg_write(format("VRAM: %d mb  of  %d mb available", nix::available_mem() / 1024, nix::total_mem() / 1024));
	}
}

void RenderPathGLForward::draw() {

	static int _frame = 0;
	_frame ++;
	if (_frame > 10) {
		if (world.ego)
			render_into_cubemap(depth_cube.get(), cube_map.get(), world.ego->pos);
		_frame = 0;
	}


	prepare_xxx();

	perf_mon->tick(PMLabel::PRE);

	prepare_lights();
	perf_mon->tick(PMLabel::PREPARE_LIGHTS);

	if (shadow_index >= 0) {
		render_shadow_map(fb_shadow.get(), 4);
		render_shadow_map(fb_shadow2.get(), 1);
	}
	perf_mon->tick(PMLabel::SHADOWS);

	cur_fb_area = rect(0, fb_main->width * resolution_scale_x, 0, fb_main->height * resolution_scale_y);
	render_into_texture(fb_main.get(), cam);

	auto source = fb_main.get();
	if (config.antialiasing_method == AntialiasingMethod::MSAA)
		source = resolve_multisampling(source);

	source = do_post_processing(source);


	nix::bind_frame_buffer(nix::FrameBuffer::DEFAULT);
	render_out(source, fb_small2->color_attachments[0]);

	draw_gui(source);
}

void RenderPathGLForward::render_into_texture(nix::FrameBuffer *fb, Camera *cam) {
	nix::bind_frame_buffer(fb);
	nix::set_viewport(cur_fb_area);
	nix::set_scissor(cur_fb_area);

	auto m = matrix::scale(1,-1,1);
	if (config.antialiasing_method == AntialiasingMethod::TAA)
		 m *= jitter(fb->width, fb->height, 0);

	// skyboxes
	float max_depth = cam->max_depth;
	cam->max_depth = 2000000;
	cam->update_matrices((float)fb->width / (float)fb->height);
	nix::set_projection_matrix(m * cam->m_projection);

	nix::clear_color(world.background);
	nix::clear_z();

	draw_skyboxes(cam);
	perf_mon->tick(PMLabel::SKYBOXES);


	// world
	cam->max_depth = max_depth;
	cam->update_matrices((float)fb->width / (float)fb->height);
	nix::set_projection_matrix(m * cam->m_projection);

	nix::bind_uniform(ubo_light, 1);
	nix::set_view_matrix(cam->m_view);
	nix::set_z(true, true);

	draw_world(true);
	plugin_manager.handle_render_inject();
	break_point();
	perf_mon->tick(PMLabel::WORLD);

	draw_particles();
	perf_mon->tick(PMLabel::PARTICLES);
	nix::set_scissor(rect::EMPTY);
}

void RenderPathGLForward::draw_particles() {
	nix::set_shader(shader_fx.get());
	nix::set_alpha(nix::Alpha::SOURCE_ALPHA, nix::Alpha::SOURCE_INV_ALPHA);
	nix::set_z(false, true);

	// particles
	auto r = matrix::rotation_q(cam->ang);
	nix::vb_temp->create_rect(rect(-1,1, -1,1));
	for (auto g: world.particle_manager->groups) {
		nix::set_texture(g->texture);
		for (auto p: g->particles)
			if (p->enabled) {
				shader_fx->set_color(shader_fx->get_location("color"), p->col);
				shader_fx->set_data(shader_fx->get_location("source"), &p->source.x1, 16);
				nix::set_model_matrix(matrix::translation(p->pos) * r * matrix::scale(p->radius, p->radius, p->radius));
				nix::draw_triangles(nix::vb_temp);
			}
	}

	// beams
	Array<vector> v;
	v.resize(6);
	nix::set_model_matrix(matrix::ID);
	for (auto g: world.particle_manager->groups) {
		nix::set_texture(g->texture);
		for (auto p: g->beams) {
			// TODO geometry shader!
			auto pa = cam->project(p->pos);
			auto pb = cam->project(p->pos + p->length);
			auto pe = vector::cross(pb - pa, vector::EZ).normalized();
			auto uae = cam->unproject(pa + pe * 0.1f);
			auto ube = cam->unproject(pb + pe * 0.1f);
			auto _e1 = (p->pos - uae).normalized() * p->radius;
			auto _e2 = (p->pos + p->length - ube).normalized() * p->radius;
			//vector e1 = -vector::cross(cam->ang * vector::EZ, p->length).normalized() * p->radius/2;
			v[0] = p->pos - _e1;
			v[1] = p->pos - _e2 + p->length;
			v[2] = p->pos + _e2 + p->length;
			v[3] = p->pos - _e1;
			v[4] = p->pos + _e2 + p->length;
			v[5] = p->pos + _e1;
			nix::vb_temp->update(0, v);
			shader_fx->set_color(shader_fx->get_location("color"), p->col);
			shader_fx->set_data(shader_fx->get_location("source"), &p->source.x1, 16);
			nix::draw_triangles(nix::vb_temp);
		}
	}


	nix::set_z(true, true);
	nix::set_alpha(nix::AlphaMode::NONE);
	break_point();
}

void RenderPathGLForward::draw_skyboxes(Camera *cam) {
	nix::set_z(false, false);
	nix::set_cull(nix::CullMode::NONE);
	nix::set_view_matrix(matrix::rotation_q(cam->ang).transpose());
	for (auto *sb: world.skybox) {
		sb->_matrix = matrix::rotation_q(sb->ang);
		nix::set_model_matrix(sb->_matrix * matrix::scale(10,10,10));
		for (int i=0; i<sb->material.num; i++) {
			set_material(sb->material[i]);
			nix::draw_triangles(sb->mesh[0]->sub[i].vertex_buffer);
		}
	}
	nix::set_cull(nix::CullMode::DEFAULT);
	break_point();
}
void RenderPathGLForward::draw_terrains(bool allow_material) {
	for (auto *t: world.terrains) {
		//nix::SetWorldMatrix(matrix::translation(t->pos));
		nix::set_model_matrix(matrix::ID);
		if (allow_material) {
			set_material(t->material);
			t->material->shader->set_data(t->material->shader->get_location("pattern0"), &t->texture_scale[0].x, 12);
			t->material->shader->set_data(t->material->shader->get_location("pattern1"), &t->texture_scale[1].x, 12);
		}
		t->draw();
		nix::draw_triangles(t->vertex_buffer);
	}
}

void RenderPathGLForward::prepare_xxx() {
	for (auto &s: world.sorted_multi) {
		ubo_multi_matrix->update_array(s.matrices);
	}
}

void RenderPathGLForward::draw_objects(bool allow_material) {
	for (auto &s: world.sorted_multi) {
		if (!s.material->cast_shadow and !allow_material)
			continue;
		Model *m = s.model;
		nix::set_model_matrix(s.matrices[0]);//m->_matrix);
		//if (allow_material)
		auto ss = s.material->shader;
		s.material->shader = shader_3d_multi;
		set_material(s.material);
		nix::bind_uniform(ubo_multi_matrix, 5);
		//msg_write(s.matrices.num);
		nix::draw_instanced_triangles(m->mesh[0]->sub[s.mat_index].vertex_buffer, s.matrices.num);
		s.material->shader = ss;
	}

	for (auto &s: world.sorted_opaque) {
		if (!s.material->cast_shadow and !allow_material)
			continue;
		Model *m = s.model;
		nix::set_model_matrix(m->_matrix);
		if (allow_material)
			set_material(s.material);
		if (m->anim.meta) {
			m->anim.mesh[0]->update_vb();
			nix::draw_triangles(m->anim.mesh[0]->sub[s.mat_index].vertex_buffer);
		} else {
			nix::draw_triangles(m->mesh[0]->sub[s.mat_index].vertex_buffer);
		}
	}

	if (allow_material)
	for (auto &s: world.sorted_trans) {
		Model *m = s.model;
		nix::set_model_matrix(m->_matrix);
		set_material(s.material);
		nix::set_cull(nix::CullMode::NONE);
		if (m->anim.meta) {
			m->anim.mesh[0]->update_vb();
			nix::draw_triangles(m->anim.mesh[0]->sub[s.mat_index].vertex_buffer);
		} else {
			nix::draw_triangles(m->mesh[0]->sub[s.mat_index].vertex_buffer);
		}
		nix::set_cull(nix::CullMode::DEFAULT);
	}
}

void RenderPathGLForward::draw_world(bool allow_material) {
	draw_terrains(allow_material);
	draw_objects(allow_material);
}

void RenderPathGLForward::prepare_lights() {

	lights.clear();
	for (auto *l: world.lights) {
		if (!l->enabled)
			continue;

		if (l->allow_shadow) {
			if (l->type == LightType::DIRECTIONAL) {
				vector center = cam->pos + cam->ang*vector::EZ * (shadow_box_size / 3.0f);
				float grid = shadow_box_size / 16;
				center.x -= fmod(center.x, grid) - grid/2;
				center.y -= fmod(center.y, grid) - grid/2;
				center.z -= fmod(center.z, grid) - grid/2;
				auto t = matrix::translation(- center);
				auto r = matrix::rotation(l->light.dir.dir2ang()).transpose();
				float f = 1 / shadow_box_size;
				auto s = matrix::scale(f, f, f);
				// map onto [-1,1]x[-1,1]x[0,1]
				shadow_proj = matrix::translation(vector(0,0,-0.5f)) * s * r * t;
			} else {
				auto t = matrix::translation(- l->light.pos);
				vector dir = - (cam->ang * vector::EZ);
				if (l->type == LightType::CONE or l->user_shadow_control)
					dir = -l->light.dir;
				auto r = matrix::rotation(dir.dir2ang()).transpose();
				//auto r = matrix::rotation(l->light.dir.dir2ang()).transpose();
				float theta = 1.35f;
				if (l->type == LightType::CONE)
					theta = l->light.theta;
				if (l->user_shadow_control)
					theta = l->user_shadow_theta;
				auto p = matrix::perspective(2 * theta, 1.0f, l->light.radius * 0.01f, l->light.radius);
				shadow_proj = p * r * t;
			}
			shadow_index = lights.num;
			l->light.proj = shadow_proj;
		}
		lights.add(l->light);
	}
	ubo_light->update_array(lights);
}

void RenderPathGLForward::render_shadow_map(nix::FrameBuffer *sfb, float scale) {
	nix::bind_frame_buffer(sfb);

	auto m = matrix::scale(scale, scale, 1);
	//m = m * jitter(sfb->width*8, sfb->height*8, 1);
	nix::set_projection_matrix(m * shadow_proj);
	nix::set_view_matrix(matrix::ID);

	nix::clear_z();

	nix::set_z(true, true);
	nix::set_shader(shader_shadow.get());


	draw_world(false);

	break_point();
}



