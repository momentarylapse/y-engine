/*
 * RenderPathGL.cpp
 *
 *  Created on: 07.08.2020
 *      Author: michi
 */

#include <GLFW/glfw3.h>

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
#include "../world/Object.h" // meh
#include "../world/Terrain.h"
#include "../world/World.h"
#include "../Config.h"
#include "../meta.h"

nix::Texture *_tex_white;

const int CUBE_SIZE = 128;

void break_point() {
	if (config.debug) {
		glFlush();
		glFinish();
	}
}

Array<Material*> post_processors;

RenderPathGL::RenderPathGL(GLFWwindow* win, int w, int h, PerformanceMonitor *pm) {
	window = win;
	glfwMakeContextCurrent(window);
	//glfwGetFramebufferSize(window, &width, &height);
	width = w;
	height = h;

	perf_mon = pm;

	nix::Init();

	shadow_box_size = config.get_float("shadow.boxsize", 2000);
	shadow_resolution = config.get_int("shadow.resolution", 1024);
	shadow_index = -1;

	ubo_light = new nix::UniformBuffer();
	tex_white = new nix::Texture();
	tex_black = new nix::Texture();
	Image im;
	im.create(16, 16, White);
	tex_white->overwrite(im);
	im.create(16, 16, Black);
	tex_black->overwrite(im);

	_tex_white = tex_white.get();

	vb_2d = new nix::VertexBuffer("3f,3f,2f");
	vb_2d->create_rect(rect(-1,1, -1,1));

	depth_cube = new nix::DepthBuffer(CUBE_SIZE, CUBE_SIZE);
	fb_cube = nullptr;
	cube_map = new nix::CubeMap(CUBE_SIZE);

	EntityManager::enabled = false;
	//shadow_cam = new Camera(v_0, quaternion::ID, rect::ID);
	EntityManager::enabled = true;
}

void RenderPathGL::render_into_cubemap(nix::DepthBuffer *depth, nix::CubeMap *cube, const vector &pos) {
	if (!fb_cube)
		fb_cube = new nix::FrameBuffer({depth});
	Camera cam(pos, quaternion::ID, rect::ID);
	cam.fov = pi/2;
	for (int i=0; i<6; i++) {
		try{
			fb_cube->update_x({cube, depth}, i);
		}catch(Exception &e){
			msg_error(e.message());
			return;
		}
		if (i == 0)
			cam.ang = quaternion::rotation(vector(0,-pi/2,0));
		if (i == 1)
			cam.ang = quaternion::rotation(vector(0,pi/2,0));
		if (i == 2)
			cam.ang = quaternion::rotation(vector(pi/2,0,pi));
		if (i == 3)
			cam.ang = quaternion::rotation(vector(-pi/2,0,pi));
		if (i == 4)
			cam.ang = quaternion::rotation(vector(0,pi,0));
		if (i == 5)
			cam.ang = quaternion::rotation(vector(0,0,0));
		render_into_texture(fb_cube.get(), &cam);
	}
}

RenderPathGLForward::RenderPathGLForward(GLFWwindow* win, int w, int h, PerformanceMonitor *pm) : RenderPathGL(win, w, h, pm) {

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
	nix::shader_dir = "";

	if (config.get_str("renderer.shader-quality", "") == "pbr")
		nix::Shader::load(hui::Application::directory_static << "forward/module-surface-pbr.shader");
	else
		nix::Shader::load(hui::Application::directory_static << "forward/module-surface.shader");

	shader_blur = nix::Shader::load(hui::Application::directory_static << "forward/blur.shader");
	shader_depth = nix::Shader::load(hui::Application::directory_static << "forward/depth.shader");
	shader_out = nix::Shader::load(hui::Application::directory_static << "forward/hdr.shader");
	//shader_3d = nix::Shader::load(hui::Application::directory_static << "forward/3d-new.shader");
	shader_3d = nix::Shader::load(hui::Application::directory_static << "default.shader");
	shader_fx = nix::Shader::load(hui::Application::directory_static << "forward/3d-fx.shader");
	//nix::default_shader_3d = shader_3d;
	shader_shadow = nix::Shader::load(hui::Application::directory_static << "forward/3d-shadow.shader");

	shader_2d = nix::Shader::load(hui::Application::directory_static << "forward/2d.shader");
	nix::shader_dir = sd;
}

nix::FrameBuffer* RenderPathGL::do_post_processing(nix::FrameBuffer *source) {
	auto cur = source;
	auto next = fb4.get();
	for (auto *m: post_processors) {
		nix::SetShader(m->shader.get());
		for (auto &u: m->uniforms)
			m->shader->set_data(u.location, u.p, u.size);
		process({cur->color_attachments[0], source->depth_buffer}, next, m->shader.get());
		cur = next;
		next = (next == fb4) ? fb5.get() : fb4.get();
	}

	if (cam->focus_enabled) {
		process_depth(cur, next, fb->depth_buffer, true);
		cur = next;
		next = (next == fb4.get()) ? fb5.get() : fb4.get();
		process_depth(fb4.get(), fb5.get(), fb->depth_buffer, false);
		cur = next;
		next = (next == fb4) ? fb5.get() : fb4.get();
	}

	process_blur(cur, fb2.get(), 1.0f, true);
	process_blur(fb2.get(), fb3.get(), 0.0f, false);
	return cur;
}

static int _frame = 0;

void RenderPathGL::start_frame() {
	nix::StartFrameGLFW(window);
}

void RenderPathGL::end_frame() {
	nix::EndFrameGLFW(window);
	break_point();
	perf_mon->tick(PMLabel::END);
}

void RenderPathGLForward::draw() {

	_frame ++;
	if (_frame > 10) {
		if (world.ego)
			render_into_cubemap(depth_cube.get(), cube_map.get(), world.ego->pos);
		_frame = 0;
	}



	perf_mon->tick(PMLabel::PRE);

	prepare_lights();
	perf_mon->tick(PMLabel::PREPARE_LIGHTS);

	if (shadow_index >= 0) {
		render_shadow_map(fb_shadow.get(), 4);
		render_shadow_map(fb_shadow2.get(), 1);
	}
	perf_mon->tick(PMLabel::SHADOWS);

	render_into_texture(fb.get(), cam);

	auto source = do_post_processing(fb.get());

	nix::BindFrameBuffer(nix::FrameBuffer::DEFAULT);

	render_out(source, fb3->color_attachments[0]);

	draw_gui(source);
}

void RenderPathGL::process_blur(nix::FrameBuffer *source, nix::FrameBuffer *target, float threshold, bool horizontal) {

	nix::SetShader(shader_blur.get());
	float r = cam->bloom_radius;
	complex ax;
	if (horizontal) {
		ax = complex(2,0);
	} else {
		ax = complex(0,1);
		//r /= 2;
	}
	shader_blur->set_float(shader_blur->get_location("radius"), r);
	shader_blur->set_float(shader_blur->get_location("threshold"), threshold / cam->exposure);
	shader_blur->set_data(shader_blur->get_location("axis"), &ax.x, 8);
	process(source->color_attachments, target, shader_blur.get());
}

void RenderPathGL::process_depth(nix::FrameBuffer *source, nix::FrameBuffer *target, nix::Texture *depth_buffer, bool horizontal) {

	nix::SetShader(shader_depth.get());
	complex ax = complex(1,0);
	if (!horizontal) {
		ax = complex(0,1);
	}
	shader_depth->set_float(shader_depth->get_location("max_radius"), 50);
	shader_depth->set_float(shader_depth->get_location("focal_length"), cam->focal_length);
	shader_depth->set_float(shader_depth->get_location("focal_blur"), cam->focal_blur);
	shader_depth->set_data(shader_depth->get_location("axis"), &ax.x, 8);
	shader_depth->set_matrix(shader_depth->get_location("invproj"), cam->m_projection.inverse());
	process({source->color_attachments[0], depth_buffer}, target, shader_depth.get());
}

void RenderPathGL::process(const Array<nix::Texture*> &source, nix::FrameBuffer *target, nix::Shader *shader) {
	nix::BindFrameBuffer(target);
	nix::SetZ(false, false);
	nix::SetProjectionOrtho(true);
	nix::SetViewMatrix(matrix::ID);
	nix::SetWorldMatrix(matrix::ID);
	//nix::SetShader(shader);

	nix::SetTextures(source);
	nix::DrawTriangles(vb_2d);
}

void RenderPathGL::draw_gui(nix::FrameBuffer *source) {
	gui::update();

	nix::SetProjectionOrtho(true);
	nix::SetCull(CULL_NONE);
	nix::SetAlpha(ALPHA_SOURCE_ALPHA, ALPHA_SOURCE_INV_ALPHA);
	nix::SetZ(false, false);

	for (auto *n: gui::sorted_nodes) {
		if (!n->eff_visible)
			continue;
		if (n->type == n->Type::PICTURE or n->type == n->Type::TEXT) {
			auto *p = (gui::Picture*)n;
			auto shader = gui::shader.get();
			if (p->shader)
				shader = p->shader.get();
			nix::SetShader(shader);
			gui::shader->set_float(shader->get_location("blur"), p->bg_blur);
			gui::shader->set_color(shader->get_location("color"), p->eff_col);
			nix::SetTextures({p->texture.get(), source->color_attachments[0]});
			nix::SetWorldMatrix(matrix::translation(vector(p->eff_area.x1, p->eff_area.y1, /*0.999f - p->eff_z/1000*/ 0.5f)) * matrix::scale(p->eff_area.width(), p->eff_area.height(), 0));
			gui::vertex_buffer->create_rect(rect::ID, p->source);
			nix::DrawTriangles(gui::vertex_buffer);
		}
	}
	nix::SetZ(true, true);
	nix::SetCull(CULL_DEFAULT);

	nix::SetAlpha(ALPHA_NONE);

	break_point();
	perf_mon->tick(PMLabel::GUI);
}

void RenderPathGL::render_out(nix::FrameBuffer *source, nix::Texture *bloom) {

	nix::SetTextures({source->color_attachments[0], bloom});
	nix::SetShader(shader_out.get());
	shader_out->set_float(shader_out->get_location("exposure"), cam->exposure);
	shader_out->set_float(shader_out->get_location("bloom_factor"), cam->bloom_factor);
	nix::SetProjectionMatrix(matrix::ID);
	nix::SetViewMatrix(matrix::ID);
	nix::SetWorldMatrix(matrix::ID);

	nix::SetZ(false, false);

	nix::DrawTriangles(vb_2d);

	break_point();
	perf_mon->tick(PMLabel::OUT);
}

void RenderPathGLForward::render_into_texture(nix::FrameBuffer *fb, Camera *cam) {
	nix::BindFrameBuffer(fb);

	float max_depth = cam->max_depth;
	cam->max_depth = 2000000;
	cam->update_matrices((float)fb->width / (float)fb->height);
	nix::SetProjectionMatrix(matrix::scale(1,-1,1) * cam->m_projection);

	nix::ResetToColor(world.background);
	nix::ResetZ();

	draw_skyboxes(cam);
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

void RenderPathGLForward::draw_particles() {
	nix::SetShader(shader_fx.get());
	nix::SetAlpha(ALPHA_SOURCE_ALPHA, ALPHA_SOURCE_INV_ALPHA);
	nix::SetZ(false, true);

	// particles
	auto r = matrix::rotation_q(cam->ang);
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

void RenderPathGLForward::draw_skyboxes(Camera *cam) {
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
void RenderPathGLForward::draw_terrains(bool allow_material) {
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
void RenderPathGLForward::draw_objects(bool allow_material) {
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
void RenderPathGLForward::draw_world(bool allow_material) {
	draw_terrains(allow_material);
	draw_objects(allow_material);
}

void RenderPathGL::set_material(Material *m) {
	auto s = m->shader.get();
	nix::SetShader(s);
	s->set_data(s->get_location("eye_pos"), &cam->pos.x, 12);
	s->set_int(s->get_location("num_lights"), lights.num);
	s->set_int(s->get_location("shadow_index"), shadow_index);
	for (auto &u: m->uniforms)
		s->set_data(u.location, u.p, u.size);

	if (m->alpha.mode == TRANSPARENCY_FUNCTIONS)
		nix::SetAlpha(m->alpha.source, m->alpha.destination);
	else if (m->alpha.mode == TRANSPARENCY_COLOR_KEY_HARD)
		nix::SetAlpha(ALPHA_COLOR_KEY_HARD);
	else
		nix::SetAlpha(ALPHA_NONE);

	set_textures(weak(m->textures));

	nix::SetMaterial(m->albedo, m->roughness, m->metal, m->emission);
}

void RenderPathGL::set_textures(const Array<nix::Texture*> &tex) {
	auto tt = tex;
	if (tt.num == 0)
		tt.add(tex_white.get());
	if (tt.num == 1)
		tt.add(tex_white.get());
	if (tt.num == 2)
		tt.add(tex_white.get());
	tt.add(fb_shadow->depth_buffer);
	tt.add(fb_shadow2->depth_buffer);
	tt.add(cube_map.get());
	nix::SetTextures(tt);
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
	nix::BindFrameBuffer(sfb);

	nix::SetProjectionMatrix(matrix::scale(scale, scale, 1) * shadow_proj);
	nix::SetViewMatrix(matrix::ID);

	nix::ResetZ();

	nix::SetZ(true, true);
	nix::SetShader(shader_shadow.get());


	draw_world(false);

	break_point();
}



