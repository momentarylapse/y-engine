/*
 * RenderPathGL.cpp
 *
 *  Created on: 07.08.2020
 *      Author: michi
 */

#include <GLFW/glfw3.h>

#include "RenderPathGLDeferred.h"
#include "RenderPathGL.h"
#include "../lib/nix/nix.h"

#include "../helper/PerformanceMonitor.h"
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
#include "../world/Terrain.h"
#include "../world/World.h"
#include "../Config.h"
#include "../meta.h"


void break_point();

extern Array<Material*> post_processors;

RenderPathGLDeferred::RenderPathGLDeferred(GLFWwindow* win, int w, int h, PerformanceMonitor *pm) : RenderPathGL(win, w, h, pm) {

	nix::Init();

	gbuffer = new nix::FrameBuffer({
		new nix::Texture(width, height, "rgba:f16"), // diffuse
		new nix::Texture(width, height, "rgba:f16"), // emission
		new nix::Texture(width, height, "rgba:f16"), // pos
		new nix::Texture(width, height, "rgba:f16"), // normal,reflection
		new nix::DepthBuffer(width, height)});

	fb = new nix::FrameBuffer({
		new nix::Texture(width, height, "rgba:f16"),
		new nix::DepthBuffer(width, height)});
	fb2 = new nix::FrameBuffer({
		new nix::Texture(width/2, height/2, "rgba:f16")});
	fb3 = new nix::FrameBuffer({
		new nix::Texture(width/2, height/2, "rgba:f16")});
	fb4 = new nix::FrameBuffer({
		new nix::Texture(width, height, "rgba:f16")});
	fb5 = new nix::FrameBuffer({
		new nix::Texture(width, height, "rgba:f16")});
	fb_shadow = new nix::FrameBuffer({
		new nix::DepthBuffer(shadow_resolution, shadow_resolution)});
	fb_shadow2 = new nix::FrameBuffer({
		new nix::DepthBuffer(shadow_resolution, shadow_resolution)});

	fb->color_attachments[0]->set_options("wrap=clamp");
	fb2->color_attachments[0]->set_options("wrap=clamp");
	fb3->color_attachments[0]->set_options("wrap=clamp");
	fb4->color_attachments[0]->set_options("wrap=clamp");
	fb5->color_attachments[0]->set_options("wrap=clamp");

	auto sd = nix::shader_dir;
	nix::Shader::load(hui::Application::directory_static << "deferred/module-surface.shader");

	shader_blur = nix::Shader::load(hui::Application::directory_static << "forward/blur.shader");
	shader_depth = nix::Shader::load(hui::Application::directory_static << "forward/depth.shader");
	shader_out = nix::Shader::load(hui::Application::directory_static << "forward/hdr.shader");
	shader_3d = nix::Shader::load(hui::Application::directory_static << "deferred/3d.shader");
	shader_gbuffer_out = nix::Shader::load(hui::Application::directory_static << "deferred/out.shader");
	shader_fx = nix::Shader::load(hui::Application::directory_static << "forward/3d-fx.shader");
	//nix::default_shader_3d = shader_3d;
	shader_shadow = nix::Shader::load(hui::Application::directory_static << "forward/3d-shadow.shader");

	shader_2d = nix::Shader::load(hui::Application::directory_static << "forward/2d.shader");
	nix::shader_dir = sd;
}

void RenderPathGLDeferred::draw() {
	perf_mon->tick(PMLabel::PRE);

	prepare_lights();
	perf_mon->tick(PMLabel::PREPARE_LIGHTS);

	if (shadow_index >= 0) {
		render_shadow_map(fb_shadow, 4);
		render_shadow_map(fb_shadow2, 1);
	}
	perf_mon->tick(PMLabel::SHADOWS);

	render_into_texture(gbuffer, cam);
	render_from_gbuffer(gbuffer, fb);

	auto source = do_post_processing(fb);

	nix::BindFrameBuffer(nix::FrameBuffer::DEFAULT);

	render_out(source, fb3->color_attachments[0]);

	draw_gui(source);
}


void RenderPathGLDeferred::render_from_gbuffer(nix::FrameBuffer *source, nix::FrameBuffer *target) {
	auto s = shader_gbuffer_out;
	nix::SetShader(s);
	s->set_data(s->get_location("eye_pos"), &cam->pos.x, 12);
	s->set_int(s->get_location("num_lights"), lights.num);
	s->set_int(s->get_location("shadow_index"), shadow_index);
	nix::BindUniform(ubo_light, 1);
	auto tex = source->color_attachments;
	tex.add(fb_shadow2->depth_buffer);
	tex.add(fb_shadow->depth_buffer);
	process(tex, target, s);
}

void RenderPathGLDeferred::render_into_texture(nix::FrameBuffer *fb, Camera *cam) {
	nix::BindFrameBuffer(fb);

	float max_depth = cam->max_depth;
	cam->max_depth = 2000000;
	cam->update_matrices((float)fb->width / (float)fb->height);
	nix::SetProjectionMatrix(matrix::scale(1,-1,1) * cam->m_projection);

	nix::ResetToColor(world.background);
	nix::ResetZ();

	draw_skyboxes();
	perf_mon->tick(PMLabel::SKYBOXES);


	cam->max_depth = max_depth;
	cam->update_matrices((float)fb->width / (float)fb->height);
	nix::SetProjectionMatrix(matrix::scale(1,-1,1) * cam->m_projection);

	nix::BindUniform(ubo_light, 1);
	nix::SetViewMatrix(cam->m_view);
	nix::SetZ(true, true);

	draw_world(true);
	plugin_manager.handle_render_inject();
	break_point();
	perf_mon->tick(PMLabel::WORLD);

	draw_particles();
	perf_mon->tick(PMLabel::PARTICLES);
}

void RenderPathGLDeferred::draw_particles() {
	nix::SetShader(shader_fx);
	nix::SetAlpha(ALPHA_SOURCE_ALPHA, ALPHA_SOURCE_INV_ALPHA);
	nix::SetZ(false, true);

	// particles
	matrix r = matrix::rotation_q(cam->ang);
	nix::vb_temp->create_rect(rect(-1,1, -1,1));
	for (auto g: world.particle_manager->groups) {
		nix::SetTexture(g->texture);
		for (auto p: g->particles)
			if (p->enabled) {
				shader_fx->set_color(shader_fx->get_location("color"), p->col);
				shader_fx->set_data(shader_fx->get_location("source"), &p->source.x1, 16);
				nix::SetWorldMatrix(matrix::translation(p->pos) * r * matrix::scale(p->radius, p->radius, p->radius));
				nix::DrawTriangles(nix::vb_temp);
			}
	}

	// beams
	Array<vector> v;
	v.resize(6);
	nix::SetWorldMatrix(matrix::ID);
	for (auto g: world.particle_manager->groups) {
		nix::SetTexture(g->texture);
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
			nix::DrawTriangles(nix::vb_temp);
		}
	}


	nix::SetZ(true, true);
	nix::SetAlpha(ALPHA_NONE);
	break_point();
}

void RenderPathGLDeferred::draw_skyboxes() {
	nix::SetZ(false, false);
	nix::SetCull(CULL_NONE);
	nix::SetViewMatrix(matrix::rotation_q(cam->ang).transpose());
	for (auto *sb: world.skybox) {
		sb->_matrix = matrix::rotation_q(sb->ang);
		nix::SetWorldMatrix(sb->_matrix * matrix::scale(10,10,10));
		for (int i=0; i<sb->material.num; i++) {
			set_material(sb->material[i]);
			nix::DrawTriangles(sb->mesh[0]->sub[i].vertex_buffer);
		}
	}
	nix::SetCull(CULL_DEFAULT);
	break_point();
}
void RenderPathGLDeferred::draw_terrains(bool allow_material) {
	for (auto *t: world.terrains) {
		//nix::SetWorldMatrix(matrix::translation(t->pos));
		nix::SetWorldMatrix(matrix::ID);
		if (allow_material) {
			set_material(t->material);
			t->material->shader->set_data(t->material->shader->get_location("pattern0"), &t->texture_scale[0].x, 12);
			t->material->shader->set_data(t->material->shader->get_location("pattern1"), &t->texture_scale[1].x, 12);
		}
		t->draw();
		nix::DrawTriangles(t->vertex_buffer);
	}
}
void RenderPathGLDeferred::draw_objects(bool allow_material) {
	for (auto &s: world.sorted_opaque) {
		if (!s.material->cast_shadow and !allow_material)
			continue;
		Model *m = s.model;
		nix::SetWorldMatrix(m->_matrix);
		if (allow_material)
			set_material(s.material);
		//nix::DrawInstancedTriangles(m->mesh[0]->sub[s.mat_index].vertex_buffer, 200);
		if (m->anim.meta) {
			m->anim.mesh[0]->update_vb();
			nix::DrawTriangles(m->anim.mesh[0]->sub[s.mat_index].vertex_buffer);
		} else {
			nix::DrawTriangles(m->mesh[0]->sub[s.mat_index].vertex_buffer);
		}
	}

	if (allow_material)
	for (auto &s: world.sorted_trans) {
		Model *m = s.model;
		nix::SetWorldMatrix(m->_matrix);
		set_material(s.material);
		//nix::DrawInstancedTriangles(m->mesh[0]->sub[s.mat_index].vertex_buffer, 200);
		nix::SetCull(CULL_NONE);
		if (m->anim.meta) {
			m->anim.mesh[0]->update_vb();
			nix::DrawTriangles(m->anim.mesh[0]->sub[s.mat_index].vertex_buffer);
		} else {
			nix::DrawTriangles(m->mesh[0]->sub[s.mat_index].vertex_buffer);
		}
		nix::SetCull(CULL_DEFAULT);
	}
}
void RenderPathGLDeferred::draw_world(bool allow_material) {
	draw_terrains(allow_material);
	draw_objects(allow_material);
}

void RenderPathGLDeferred::prepare_lights() {

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

void RenderPathGLDeferred::render_shadow_map(nix::FrameBuffer *sfb, float scale) {
	nix::BindFrameBuffer(sfb);

	nix::SetProjectionMatrix(matrix::scale(scale, scale, 1) * shadow_proj);
	nix::SetViewMatrix(matrix::ID);

	nix::ResetZ();

	nix::SetZ(true, true);
	nix::SetShader(shader_shadow);


	draw_world(false);

	break_point();
}



