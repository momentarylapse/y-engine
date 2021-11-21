/*
 * RenderPathVulkan.cpp
 *
 *  Created on: Nov 18, 2021
 *      Author: michi
 */

#include "RenderPathVulkan.h"
#ifdef USING_VULKAN
#include "../graphics-impl.h"
#include "../lib/image/image.h"
#include "../lib/math/vector.h"
#include "../lib/math/complex.h"
#include "../lib/math/rect.h"
#include "../lib/file/msg.h"
#include "../helper/PerformanceMonitor.h"
#include "../helper/ResourceManager.h"
#include "../plugins/PluginManager.h"
#include "../fx/Particle.h"
#include "../fx/Beam.h"
#include "../fx/ParticleManager.h"
#include "../gui/gui.h"
#include "../gui/Picture.h"
#include "../world/Camera.h"
#include "../world/Material.h"
#include "../world/Model.h"
#include "../world/Object.h" // meh
#include "../world/Terrain.h"
#include "../world/World.h"
#include "../world/Light.h"
#include "../world/Entity3D.h"
#include "../world/components/Animator.h"
#include "../Config.h"
#include "../meta.h"


UniformBuffer *ubo_multi_matrix = nullptr;

Texture *_tex_white;

const int CUBE_SIZE = 128;

void break_point() {
	if (config.debug) {
		glFlush();
		glFinish();
	}
}



struct UBO {
	matrix m,v,p;
	color albedo, emission;
	float roughness, metal;
	int num_lights;
};

struct UBOGUI {
	matrix m,v,p;
	color col;
	rect source;
	float blur, exposure, gamma;
};


const float HALTON2[] = {1/2.0f, 1/4.0f, 3/4.0f, 1/8.0f, 5/8.0f, 3/8.0f, 7/8.0f, 1/16.0f, 9/16.0f, 3/16.0f, 11/16.0f, 5/16.0f, 13/16.0f};
const float HALTON3[] = {1/3.0f, 2/3.0f, 1/9.0f, 4/9.0f, 7/9.0f, 2/9.0f, 5/9.0f, 8/9.0f, 1/27.0f, 10/27.0f, 19/27.0f, 2/27.0f, 11/27.0f, 20/27.0f};

static int jitter_frame = 0;

matrix jitter(float w, float h, int uid) {
	int u = (jitter_frame + uid * 2) % 13;
	int v = (jitter_frame + uid * 5) % 14;
	return matrix::translation(vector((HALTON2[u] - 0.5f) / w, (HALTON3[v] - 0.5f) / h, 0));
}

void jitter_iterate() {
	jitter_frame ++;
}

void create_quad(VertexBuffer *vb, const rect &r, const rect &s = rect::ID) {
	vb->build_v3_v3_v2_i({
		{{r.x1,r.y1,0}, {0,0,1}, s.x1,s.y1},
		{{r.x2,r.y1,0}, {0,0,1}, s.x2,s.y1},
		{{r.x1,r.y2,0}, {0,0,1}, s.x1,s.y2},
		{{r.x2,r.y2,0}, {0,0,1}, s.x2,s.y2}}, {0,1,3, 0,3,2});
}

RenderPathVulkan::RenderPathVulkan(GLFWwindow* win, int w, int h, RenderPathType _type) {
	type = _type;
	window = win;
	glfwMakeContextCurrent(window);
	//glfwGetFramebufferSize(window, &width, &height);
	width = w;
	height = h;

	using_view_space = true;


	instance = vulkan::init(window, {"glfw", "validation", "api=1.2"});


	image_available_semaphore = new vulkan::Semaphore();
	render_finished_semaphore = new vulkan::Semaphore();


	framebuffer_resized = false;
	pool = new vulkan::DescriptorPool("buffer:1024,sampler:1024", 1024);

	_create_swap_chain_and_stuff();


	/*nix::allow_separate_vertex_arrays = true;
	nix::init();*/

	shadow_box_size = config.get_float("shadow.boxsize", 2000);
	shadow_resolution = config.get_int("shadow.resolution", 1024);
	shadow_index = -1;

	ubo_light = new UniformBuffer(1024 * sizeof(UBOLight));
	tex_white = new Texture();
	tex_black = new Texture();
	Image im;
	im.create(16, 16, White);
	tex_white->override(im);
	im.create(16, 16, Black);
	tex_black->override(im);

	_tex_white = tex_white.get();

	/*vb_2d = new nix::VertexBuffer("3f,3f,2f|i");
	vb_2d->create_rect(rect(-1,1, -1,1));*/

	/*depth_cube = new nix::DepthBuffer(CUBE_SIZE, CUBE_SIZE, "d24s8");
	fb_cube = nullptr;
	cube_map = new nix::CubeMap(CUBE_SIZE, "rgba:i8");*/

	//shadow_cam = new Camera(v_0, quaternion::ID, rect::ID);

	material_shadow = new Material;
	material_shadow->shader_path = "shadow.shader";

	//ubo_multi_matrix = new nix::UniformBuffer();*/





	shader_2d = ResourceManager::load_shader("vulkan/2d.shader");
	pipeline_gui = new vulkan::Pipeline(shader_2d, default_render_pass(), 0, 1);
	pipeline_gui->set_blend(Alpha::SOURCE_ALPHA, Alpha::SOURCE_INV_ALPHA);
	pipeline_gui->set_z(false, false);
	pipeline_gui->rebuild();


	vb_gui = new VertexBuffer();
	create_quad(vb_gui, rect::ID);




	ResourceManager::default_shader = "default.shader";
	/*if (config.get_str("renderer.shader-quality", "") == "pbr") {
		ResourceManager::load_shader("module-lighting-pbr.shader");
		ResourceManager::load_shader("forward/module-surface-pbr.shader");
	} else {
		ResourceManager::load_shader("forward/module-surface.shader");
	}
	ResourceManager::load_shader("module-vertex-default.shader");
	ResourceManager::load_shader("module-vertex-animated.shader");
	ResourceManager::load_shader("module-vertex-instanced.shader");*/
	ResourceManager::load_shader("vulkan/module-surface-dummy.shader");
	ResourceManager::load_shader("module-vertex-default.shader");
}

RenderPathVulkan::~RenderPathVulkan() {
	delete instance;
}



void RenderPathVulkan::_create_swap_chain_and_stuff() {
	swap_chain = new vulkan::SwapChain(window);
	auto swap_images = swap_chain->create_textures();
	for (auto t: swap_images)
		wait_for_frame_fences.add(new vulkan::Fence());

	for (auto t: swap_images)
		command_buffers.add(new CommandBuffer());

	depth_buffer = swap_chain->create_depth_buffer();
	_default_render_pass = swap_chain->create_render_pass(depth_buffer);
	frame_buffers = swap_chain->create_frame_buffers(_default_render_pass, depth_buffer);
	width = swap_chain->width;
	height = swap_chain->height;
}


/*func _delete_swap_chain_and_stuff()
	for fb in frame_buffers
		del fb
	del _default_render_pass
	del depth_buffer
	del swap_chain*/

void RenderPathVulkan::rebuild_default_stuff() {
	msg_write("recreate swap chain");

	vulkan::default_device->wait_idle();

	//_delete_swap_chain_and_stuff();
	_create_swap_chain_and_stuff();
}



RenderPass* RenderPathVulkan::default_render_pass() const {
	return _default_render_pass;
}

FrameBuffer* RenderPathVulkan::current_frame_buffer() const {
	return frame_buffers[image_index];
}

CommandBuffer* RenderPathVulkan::current_command_buffer() const {
	return command_buffers[image_index];
}

rect RenderPathVulkan::area() const {
	return rect(0, frame_buffers[0]->width, 0, frame_buffers[0]->height);
}

void RenderPathVulkan::render_into_cubemap(DepthBuffer *depth, CubeMap *cube, const vector &pos) {
	/*if (!fb_cube)
		fb_cube = new nix::FrameBuffer({depth});
	Entity3D o(pos, quaternion::ID);
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
			o.ang = quaternion::rotation(vector(0,-pi/2,0));
		if (i == 1)
			o.ang = quaternion::rotation(vector(0,pi/2,0));
		if (i == 2)
			o.ang = quaternion::rotation(vector(pi/2,0,pi));
		if (i == 3)
			o.ang = quaternion::rotation(vector(-pi/2,0,pi));
		if (i == 4)
			o.ang = quaternion::rotation(vector(0,pi,0));
		if (i == 5)
			o.ang = quaternion::rotation(vector(0,0,0));
		prepare_lights(&cam);
		render_into_texture(fb_cube.get(), &cam, fb_cube->area());
	}
	cam.owner = nullptr;*/
}

rect RenderPathVulkan::dynamic_fb_area() const {
	return rect(0, fb_main->width * resolution_scale_x, 0, fb_main->height * resolution_scale_y);
}

FrameBuffer *RenderPathVulkan::next_fb(FrameBuffer *cur) {
	return (cur == fb2) ? fb3.get() : fb2.get();
}



// GTX750: 1920x1080 0.277 ms per trivial step
FrameBuffer* RenderPathVulkan::do_post_processing(FrameBuffer *source) {
	/*PerformanceMonitor::begin(ch_post);
	auto cur = source;

	// scripts
	for (auto &p: post_processors) {
		PerformanceMonitor::begin(p.channel);
		cur = (*p.func)(cur);
		break_point();
		PerformanceMonitor::end(p.channel);
	}


	if (cam->focus_enabled) {
		PerformanceMonitor::begin(ch_post_focus);
		auto next = next_fb(cur);
		process_depth(cur, next, complex(1,0));
		cur = next;
		next = next_fb(cur);
		process_depth(cur, next, complex(0,1));
		cur = next;
		break_point();
		PerformanceMonitor::end(ch_post_focus);
	}

	// render blur into fb3!
	PerformanceMonitor::begin(ch_post_blur);
	process_blur(cur, fb_small1.get(), 1.0f, complex(2,0));
	process_blur(fb_small1.get(), fb_small2.get(), 0.0f, complex(0,1));
	break_point();
	PerformanceMonitor::end(ch_post_blur);

	PerformanceMonitor::end(ch_post);
	return cur;*/
	return nullptr;
}

FrameBuffer* RenderPathVulkan::resolve_multisampling(FrameBuffer *source) {
	/*auto next = next_fb(source);
	if (true) {
		shader_resolve_multisample->set_float("width", source->width);
		shader_resolve_multisample->set_float("height", source->height);
		process({source->color_attachments[0].get(), depth_buffer}, next, shader_resolve_multisample.get());
	} else {
		// not sure, why this does not work... :(
		nix::resolve_multisampling(next, source);
	}
	return next;*/
	return nullptr;
}


bool RenderPathVulkan::start_frame() {

	if (!swap_chain->acquire_image(&image_index, image_available_semaphore)) {
		rebuild_default_stuff();
		return false;
	}

	auto f = wait_for_frame_fences[image_index];
	f->wait();
	f->reset();


	auto cb = current_command_buffer();
	auto rp = default_render_pass();
	auto fb = current_frame_buffer();

	prepare_gui(fb);
	prepare_lights(cam);


	cb->begin();

	cb->set_viewport(area());

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


	return true;
}

void RenderPathVulkan::end_frame() {
	PerformanceMonitor::begin(ch_end);


	auto f = wait_for_frame_fences[image_index];
	vulkan::default_device->present_queue.submit(command_buffers[image_index], {image_available_semaphore}, {render_finished_semaphore}, f);

	swap_chain->present(image_index, {render_finished_semaphore});

	vulkan::default_device->wait_idle();
	PerformanceMonitor::end(ch_end);
}


void RenderPathVulkan::process_blur(FrameBuffer *source, FrameBuffer *target, float threshold, const complex &axis) {
	/*float r = cam->bloom_radius * resolution_scale_x;
	shader_blur->set_float("radius", r);
	shader_blur->set_float("threshold", threshold / cam->exposure);
	shader_blur->set_floats("axis", &axis.x, 2);
	process(weak(source->color_attachments), target, shader_blur.get());*/
}

void RenderPathVulkan::process_depth(FrameBuffer *source, FrameBuffer *target, const complex &axis) {
	/*shader_depth->set_float("max_radius", 50);
	shader_depth->set_float("focal_length", cam->focal_length);
	shader_depth->set_float("focal_blur", cam->focal_blur);
	shader_depth->set_floats("axis", &axis.x, 2);
	shader_depth->set_matrix("invproj", cam->m_projection.inverse());
	process({source->color_attachments[0].get(), depth_buffer}, target, shader_depth.get());*/
}

void RenderPathVulkan::process(const Array<Texture*> &source, FrameBuffer *target, Shader *shader) {
	/*nix::bind_frame_buffer(target);
	nix::set_scissor(rect(0, target->width*resolution_scale_x, 0, target->height*resolution_scale_y));
	nix::set_z(false, false);
	//nix::set_projection_ortho_relative();
	//nix::set_view_matrix(matrix::ID);
	//nix::set_model_matrix(matrix::ID);
	shader->set_floats("resolution_scale", &resolution_scale_x, 2);
	nix::set_shader(shader);

	nix::set_textures(source);
	nix::draw_triangles(vb_2d);
	nix::set_scissor(rect::EMPTY);*/
}

void RenderPathVulkan::prepare_gui(FrameBuffer *source) {
	PerformanceMonitor::begin(ch_gui);
	gui::update();

	UBOGUI ubo;
	ubo.v = matrix::ID;
	ubo.p = matrix::scale(2.0f, 2.0f, 1) * matrix::translation(vector(-0.5f, -0.5f, 0)); // nix::set_projection_ortho_relative()
	ubo.gamma = 2.2f;
	ubo.exposure = 1.0f;

	int index = 0;

	for (auto *n: gui::sorted_nodes) {
		if (!n->eff_visible)
			continue;
		if (n->type == n->Type::PICTURE or n->type == n->Type::TEXT) {
			auto *p = (gui::Picture*)n;

			if (index >= ubo_gui.num) {
				dset_gui.add(pool->create_set("buffer,sampler"));
				ubo_gui.add(new UniformBuffer(sizeof(UBOGUI)));
			}

			if (p->angle == 0) {
				ubo.m = matrix::translation(vector(p->eff_area.x1, p->eff_area.y1, /*0.999f - p->eff_z/1000*/ 0.5f)) * matrix::scale(p->eff_area.width(), p->eff_area.height(), 0);
			} else {
				// TODO this should use the physical ratio
				float r = (float)width / (float)height;
				ubo.m = matrix::translation(vector(p->eff_area.x1, p->eff_area.y1, /*0.999f - p->eff_z/1000*/ 0.5f)) * matrix::scale(1/r, 1, 0) * matrix::rotation_z(p->angle) * matrix::scale(p->eff_area.width() * r, p->eff_area.height(), 0);
			}
			ubo.blur = p->bg_blur;
			ubo.col = p->eff_col;
			ubo.source = p->source;
			ubo_gui[index]->update(&ubo);

			dset_gui[index]->set_buffer(0, ubo_gui[index]);
			dset_gui[index]->set_texture(1, p->texture.get());
//			dset_gui[index]->set_texture(2, source->...);
			dset_gui[index]->update();
			index ++;
		}
	}
	PerformanceMonitor::end(ch_gui);
}

void RenderPathVulkan::draw_gui(CommandBuffer *cb) {
	PerformanceMonitor::begin(ch_gui);

	cb->bind_pipeline(pipeline_gui);

	int index = 0;
	for (auto *n: gui::sorted_nodes) {
		if (!n->eff_visible)
			continue;
		if (n->type == n->Type::PICTURE or n->type == n->Type::TEXT) {

			cb->bind_descriptor_set(0, dset_gui[index]);
			cb->draw(vb_gui);
			index ++;
		}
	}

	//break_point();
	PerformanceMonitor::end(ch_gui);
}

void RenderPathVulkan::render_out(FrameBuffer *source, Texture *bloom) {
	/*PerformanceMonitor::begin(ch_out);

	nix::set_textures({source->color_attachments[0].get(), bloom});
	nix::set_shader(shader_out.get());
	shader_out->set_float("exposure", cam->exposure);
	shader_out->set_float("bloom_factor", cam->bloom_factor);
	shader_out->set_float("scale_x", resolution_scale_x);
	shader_out->set_float("scale_y", resolution_scale_y);
	nix::set_projection_matrix(matrix::ID);
	nix::set_view_matrix(matrix::ID);
	nix::set_model_matrix(matrix::ID);

	nix::set_z(false, false);

	nix::draw_triangles(vb_2d);

	break_point();
	PerformanceMonitor::end(ch_out);*/
}

Map<Shader*,Pipeline*> ob_pipelines;
Pipeline *get_pipeline(Shader *s, RenderPass *rp) {
	if (ob_pipelines.contains(s))
		return ob_pipelines[s];
	msg_write("NEW PIPELINE");
	auto p = new Pipeline(s, rp, 0, 1);
	ob_pipelines.add({s, p});
	return p;
}
Map<Shader*,Pipeline*> ob_pipelines_alpha;
Pipeline *get_pipeline_alpha(Shader *s, RenderPass *rp, Alpha src, Alpha dst) {
	if (ob_pipelines_alpha.contains(s))
		return ob_pipelines_alpha[s];
	msg_write(format("NEW PIPELINE ALPHA %d %d", (int)src, (int)dst));
	auto p = new Pipeline(s, rp, 0, 1);
	p->set_z(false, false);
	p->set_blend(src, dst);
	//p->set_culling(0);
	p->rebuild();
	ob_pipelines_alpha.add({s, p});
	return p;
}

void RenderPathVulkan::set_material(CommandBuffer *cb, DescriptorSet *dset, Material *m, RenderPathType t, ShaderVariant v) {
	auto s = m->get_shader((int)t-1, v);
	Pipeline *p;

	if (m->alpha.mode == TransparencyMode::FUNCTIONS) {
		p = get_pipeline_alpha(s, default_render_pass(), m->alpha.source, m->alpha.destination);
		//msg_write(format("a %d %d  %s  %s", (int)m->alpha.source, (int)m->alpha.destination, p2s(s), p2s(p)));
	} else if (m->alpha.mode == TransparencyMode::COLOR_KEY_HARD) {
		msg_write("HARD");
		p = get_pipeline_alpha(s, default_render_pass(), Alpha::SOURCE_ALPHA, Alpha::SOURCE_INV_ALPHA);
	} else {
		p = get_pipeline(s, default_render_pass());
	}

	cb->bind_pipeline(p);

	set_textures(dset, 2, m->textures.num, weak(m->textures));
	dset->update();
	cb->bind_descriptor_set(0, dset);


	/*nix::set_shader(s);
	if (using_view_space)
		s->set_floats("eye_pos", &cam->get_owner<Entity3D>()->pos.x, 3);
	else
		s->set_floats("eye_pos", &vector::ZERO.x, 3);
	s->set_int("num_lights", lights.num);
	s->set_int("shadow_index", shadow_index);
	for (auto &u: m->uniforms)
		s->set_floats(u.name, u.p, u.size/4);

	if (m->alpha.mode == TransparencyMode::FUNCTIONS)
		nix::set_alpha(m->alpha.source, m->alpha.destination);
	else if (m->alpha.mode == TransparencyMode::COLOR_KEY_HARD)
		nix::set_alpha(nix::Alpha::SOURCE_ALPHA, nix::Alpha::SOURCE_INV_ALPHA);
	else
		nix::disable_alpha();

	set_textures(weak(m->textures));

	nix::set_material(m->albedo, m->roughness, m->metal, m->emission);*/
}

void RenderPathVulkan::set_textures(DescriptorSet *dset, int i0, int n, const Array<Texture*> &tex) {
	/*auto tt = tex;
	if (tt.num == 0)
		tt.add(tex_white.get());
	if (tt.num == 1)
		tt.add(tex_white.get());
	if (tt.num == 2)
		tt.add(tex_white.get());
	//tt.add(fb_shadow->depth_buffer.get());
	//tt.add(fb_shadow2->depth_buffer.get());
	//tt.add(cube_map.get());
	foreachi (auto t, tt, i)
		dset->set_texture(i0 + i, t);*/
	for (int k=0; k<n; k++) {
		dset->set_texture(i0 + k, _tex_white);
		if (k < tex.num)
			if (tex[k])
				dset->set_texture(i0 + k, tex[k]);
	}
}




void RenderPathVulkan::draw_particles() {
	/*PerformanceMonitor::begin(ch_fx);
	nix::set_shader(shader_fx.get());
	nix::set_alpha(nix::Alpha::SOURCE_ALPHA, nix::Alpha::SOURCE_INV_ALPHA);
	nix::set_z(false, true);

	// particles
	auto r = matrix::rotation_q(cam->get_owner<Entity3D>()->ang);
	nix::vb_temp->create_rect(rect(-1,1, -1,1));
	for (auto g: world.particle_manager->groups) {
		nix::set_texture(g->texture);
		for (auto p: g->particles)
			if (p->enabled) {
				shader_fx->set_color("color", p->col);
				shader_fx->set_floats("source", &p->source.x1, 4);
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
			shader_fx->set_color("color", p->col);
			shader_fx->set_floats("source", &p->source.x1, 4);
			nix::draw_triangles(nix::vb_temp);
		}
	}

	// script injectors
	for (auto &i: fx_injectors)
		(*i.func)();


	nix::set_z(true, true);
	nix::disable_alpha();
	break_point();
	PerformanceMonitor::end(ch_fx);*/
}

static Array<UniformBuffer*> sb_ubos;
static Array<DescriptorSet*> sb_dsets;

void RenderPathVulkan::draw_skyboxes(CommandBuffer *cb, Camera *cam) {

	int index = 0;
	UBO ubo;

	float max_depth = cam->max_depth;
	cam->max_depth = 2000000;
	cam->update_matrices((float)width / (float)height);

	ubo.p = cam->m_projection;
	ubo.v = matrix::rotation_q(cam->get_owner<Entity3D>()->ang).transpose();
	ubo.m = matrix::ID;
	ubo.num_lights = world.lights.num;

	//nix::set_z(false, false);
	//nix::set_cull(nix::CullMode::NONE);
	for (auto *sb: world.skybox) {
		sb->_matrix = matrix::rotation_q(sb->get_owner<Entity3D>()->ang);
		ubo.m = sb->_matrix * matrix::scale(10,10,10);


		for (int i=0; i<sb->material.num; i++) {
			if (index >= sb_ubos.num) {
				sb_ubos.add(new UniformBuffer(sizeof(UBO)));
				sb_dsets.add(pool->create_set(sb->material[i]->get_shader((int)type-1, ShaderVariant::DEFAULT)));
			}
			ubo.albedo = sb->material[i]->albedo;
			ubo.emission = sb->material[i]->emission;
			ubo.metal = sb->material[i]->metal;
			ubo.roughness = sb->material[i]->roughness;

			sb_ubos[index]->update(&ubo);
			sb_dsets[index]->set_buffer(0, sb_ubos[index]);
			sb_dsets[index]->set_buffer(1, ubo_light);

			set_material(cb, sb_dsets[index], sb->material[i], type, ShaderVariant::DEFAULT);
			cb->draw(sb->mesh[0]->sub[i].vertex_buffer);

			index ++;
		}
	}


	cam->max_depth = max_depth;

	//nix::set_cull(nix::CullMode::DEFAULT);
	//nix::disable_alpha();
	//break_point();*/
}


static Array<UniformBuffer*> tr_ubos;
static Array<DescriptorSet*> tr_dsets;

void RenderPathVulkan::draw_terrains(CommandBuffer *cb, bool allow_material) {
	int index = 0;
	UBO ubo;

	cam->update_matrices((float)width / (float)height);
	ubo.p = cam->m_projection;
	ubo.v = cam->m_view;
	ubo.m = matrix::ID;
	ubo.num_lights = world.lights.num;

	for (auto *t: world.terrains) {
		auto o = t->get_owner<Entity3D>();
		ubo.m = matrix::translation(o->pos);
		ubo.albedo = t->material->albedo;
		ubo.emission = t->material->emission;
		ubo.metal = t->material->metal;
		ubo.roughness = t->material->roughness;

		if (index >= tr_ubos.num) {
			tr_ubos.add(new UniformBuffer(sizeof(UBO)));
			tr_dsets.add(pool->create_set(t->material->get_shader((int)type-1, ShaderVariant::DEFAULT)));
		}

		tr_ubos[index]->update(&ubo);
		tr_dsets[index]->set_buffer(0, tr_ubos[index]);
		tr_dsets[index]->set_buffer(1, ubo_light);

		if (allow_material) {
			set_material(cb, tr_dsets[index], t->material, type, ShaderVariant::DEFAULT);
			cb->push_constant(0, 4, &t->texture_scale[0].x);
			cb->push_constant(4, 4, &t->texture_scale[1].x);
		}
		t->prepare_draw(cam->get_owner<Entity3D>()->pos);
		cb->draw(t->vertex_buffer);
		index ++;
	}
}

void RenderPathVulkan::draw_objects_instanced(bool allow_material) {
	/*for (auto &s: world.sorted_multi) {
		if (!s.material->cast_shadow and !allow_material)
			continue;
		Model *m = s.model;
		nix::set_model_matrix(s.matrices[0]);//m->_matrix);
		if (allow_material)
			set_material(s.material, type, ShaderVariant::INSTANCED);
		else
			set_material(material_shadow, type, ShaderVariant::INSTANCED);
		nix::bind_buffer(ubo_multi_matrix, 5);
		//msg_write(s.matrices.num);
		nix::draw_instanced_triangles(m->mesh[0]->sub[s.mat_index].vertex_buffer, s.matrices.num);
		//s.material->shader = ss;
	}*/
}

static Array<UniformBuffer*> ob_ubos;
static Array<DescriptorSet*> ob_dsets;

void RenderPathVulkan::draw_objects_opaque(CommandBuffer *cb, bool allow_material) {
	int index = 0;
	UBO ubo;

	cam->update_matrices((float)width / (float)height);
	ubo.p = cam->m_projection;
	ubo.v = cam->m_view;
	ubo.m = matrix::ID;
	ubo.num_lights = world.lights.num;

	cam->update_matrices((float)width / (float)height);
	ubo.p = cam->m_projection;
	ubo.v = cam->m_view;

	for (auto &s: world.sorted_opaque) {
		if (!s.material->cast_shadow and !allow_material)
			continue;
		Model *m = s.model;

		if (index >= ob_ubos.num) {
			ob_ubos.add(new UniformBuffer(sizeof(UBO)));
			ob_dsets.add(pool->create_set(s.material->get_shader((int)type-1, ShaderVariant::DEFAULT)));
			ob_dsets[index]->set_buffer(1, ubo_light);
		}

		m->update_matrix();
		ubo.m = m->_matrix;
		ubo.albedo = s.material->albedo;
		ubo.emission = s.material->emission;
		ubo.metal = s.material->metal;
		ubo.roughness = s.material->roughness;
		ob_ubos[index]->update(&ubo);
		ob_dsets[index]->set_buffer(0, ob_ubos[index]);

		auto ani = m->owner ? m->owner->get_component<Animator>() : nullptr;

		if (ani) {
			set_material(cb, ob_dsets[index], s.material, type, ShaderVariant::DEFAULT);


			/*if (allow_material)
				set_material(s.material, type, ShaderVariant::ANIMATED);
			else
				set_material(material_shadow, type, ShaderVariant::ANIMATED);
			ani->buf->update_array(ani->dmatrix);
			nix::bind_buffer(ani->buf, 7);*/
		} else {
			if (allow_material)
				set_material(cb, ob_dsets[index], s.material, type, ShaderVariant::DEFAULT);
			else
				set_material(cb, ob_dsets[index], material_shadow, type, ShaderVariant::DEFAULT);
		}

		cb->draw(m->mesh[0]->sub[s.mat_index].vertex_buffer);
		index ++;
	}
}

void RenderPathVulkan::draw_objects_transparent(bool allow_material, RenderPathType t) {
	/*nix::set_z(false, true);
	if (allow_material)
	for (auto &s: world.sorted_trans) {
		Model *m = s.model;
		m->update_matrix();
		nix::set_model_matrix(m->_matrix);
		nix::set_cull(nix::CullMode::NONE);
		auto ani = m->owner ? m->owner->get_component<Animator>() : nullptr;
		if (ani) {
			set_material(s.material, t, ShaderVariant::ANIMATED);
		} else {
			set_material(s.material, t, ShaderVariant::DEFAULT);
		}
		nix::draw_triangles(m->mesh[0]->sub[s.mat_index].vertex_buffer);
		nix::set_cull(nix::CullMode::DEFAULT);
	}
	nix::disable_alpha();
	nix::set_z(true, true);*/
}


void RenderPathVulkan::prepare_instanced_matrices() {
	/*PerformanceMonitor::begin(ch_pre);
	for (auto &s: world.sorted_multi) {
		ubo_multi_matrix->update_array(s.matrices);
	}
	PerformanceMonitor::end(ch_pre);*/
}

void RenderPathVulkan::prepare_lights(Camera *cam) {
	PerformanceMonitor::begin(ch_prepare_lights);

	lights.clear();
	for (auto *l: world.lights) {
		if (!l->enabled)
			continue;

		l->update(cam, shadow_box_size, using_view_space);

		if (l->allow_shadow) {
			shadow_index = lights.num;
			shadow_proj = l->shadow_projection;
		}
		lights.add(l->light);
	}
	ubo_light->update_part(&lights[0], 0, lights.num * sizeof(lights[0]));
	PerformanceMonitor::end(ch_prepare_lights);
}

#endif



#if 0
RenderPathVulkan::RenderPathVulkan(RendererVulkan *r, PerformanceMonitor *pm, const string &shadow_shader_filename, const string &fx_shader_filename) {
	renderer = r;
	perf_mon = pm;


	shader_fx = vulkan::Shader::load(fx_shader_filename);
	pipeline_fx = new vulkan::Pipeline(shader_fx, renderer->default_render_pass(), 0, 1);
	pipeline_fx->set_blend(VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
	pipeline_fx->set_z(true, false);
	pipeline_fx->set_culling(0);
	pipeline_fx->rebuild();

	particle_vb = new vulkan::VertexBuffer();
	Array<vulkan::Vertex1> vertices;
	vertices.add({vector(-1,-1,0), vector::EZ, 0,0});
	vertices.add({vector(-1, 1,0), vector::EZ, 0,1});
	vertices.add({vector( 1,-1,0), vector::EZ, 1,0});
	vertices.add({vector( 1, 1,0), vector::EZ, 1,1});
	particle_vb->build1i(vertices, {0,2,1, 1,2,3});


	shadow_renderer = new ShadowMapRenderer(shadow_shader_filename);


	AllowXContainer = false;
	light_cam = new Camera(v_0, quaternion::ID, rect::ID);
	AllowXContainer = true;
}

RenderPathVulkan::~RenderPathVulkan() {
	delete particle_vb;
	delete pipeline_fx;

	delete shadow_renderer;

	delete light_cam;
}

void RenderPathVulkan::pick_shadow_source() {

}

void RenderPathVulkan::draw_world(vulkan::CommandBuffer *cb, int light_index) {

	GeoPush gp;
	gp.eye_pos = cam->pos;

	for (auto *t: world.terrains) {
		gp.model = matrix::ID;
		gp.emission = Black;
		gp.xxx[0] = 0.0f;
		cb->push_constant(0, sizeof(gp), &gp);
		cb->bind_descriptor_set_dynamic(0, t->dset, {light_index});
		cb->draw(t->vertex_buffer);
	}

	for (auto &s: world.sorted_opaque) {
		Model *m = s.model;
		gp.model = mtr(m->pos, m->ang);
		gp.emission = s.material->emission;
		gp.xxx[0] = 0.2f;
		cb->push_constant(0, sizeof(gp), &gp);

		cb->bind_descriptor_set_dynamic(0, s.dset, {light_index});
		cb->draw(m->mesh[0]->sub[0].vertex_buffer);
	}

}

void RenderPathVulkan::prepare_all(Renderer *r, Camera *c) {

	c->set_view((float)r->width / (float)r->height);

	UBOMatrices u;
	u.proj = c->m_projection;
	u.view = c->m_view;

	UBOFog f;
	f.col = world.fog._color;
	f.distance = world.fog.distance;
	world.ubo_fog->update(&f);

	for (auto *t: world.terrains) {
		u.model = matrix::ID;
		t->ubo->update(&u);
		//t->dset->set({t->ubo, world.ubo_light, world.ubo_fog}, {t->material->textures[0], tex_white, tex_black, shadow_renderer->depth_buffer});

		t->draw(); // rebuild stuff...
	}
	for (auto &s: world.sorted_opaque) {
		Model *m = s.model;

		u.model = mtr(m->pos, m->ang);
		s.ubo->update(&u);
	}

	gui::update();
}


void RenderPathVulkan::render_into_shadow(ShadowMapRenderer *r) {
	r->start_frame();
	auto *cb = r->cb;

	cb->begin_render_pass(r->default_render_pass(), r->current_frame_buffer());
	cb->set_pipeline(r->pipeline);
	cb->set_viewport(r->area());

	draw_world(cb, 0);
	cb->end_render_pass();

	r->end_frame();
}
#endif

