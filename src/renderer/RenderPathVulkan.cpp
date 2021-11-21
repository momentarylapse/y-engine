/*
 * RenderPathVulkan.cpp
 *
 *  Created on: Nov 18, 2021
 *      Author: michi
 */

#include "RenderPathVulkan.h"
#ifdef USING_VULKAN
#include "base.h"
#include "RendererVulkan.h"
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

const int CUBE_SIZE = 128;




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

struct UBOBlur{
	vec2 axis;
	//float dummy1[2];
	float radius;
	float threshold;
	float dummy2[2];
	float kernel[20];
};
UniformBuffer *blur_ubo[2];
DescriptorSet *blur_dset[2];
Pipeline *blur_pipeline;
RenderPass *blur_render_pass;


void create_quad(VertexBuffer *vb, const rect &r, const rect &s = rect::ID) {
	vb->build_v3_v3_v2_i({
		{{r.x1,r.y1,0}, {0,0,1}, s.x1,s.y1},
		{{r.x2,r.y1,0}, {0,0,1}, s.x2,s.y1},
		{{r.x1,r.y2,0}, {0,0,1}, s.x1,s.y2},
		{{r.x2,r.y2,0}, {0,0,1}, s.x2,s.y2}}, {0,1,3, 0,3,2});
}

RenderPathVulkan::RenderPathVulkan(RendererVulkan *r, RenderPathType _type) {
	type = _type;
	renderer = r;
	width = renderer->width;
	height = renderer->height;

	vb_2d = nullptr;

	using_view_space = true;


	shadow_box_size = config.get_float("shadow.boxsize", 2000);
	shadow_resolution = config.get_int("shadow.resolution", 1024);
	shadow_index = -1;

	ubo_light = new UniformBuffer(1024 * sizeof(UBOLight));

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
	pipeline_gui = new vulkan::Pipeline(shader_2d, renderer->default_render_pass(), 0, 1);
	pipeline_gui->set_blend(Alpha::SOURCE_ALPHA, Alpha::SOURCE_INV_ALPHA);
	pipeline_gui->set_z(false, false);
	pipeline_gui->rebuild();


	vb_gui = new VertexBuffer();
	create_quad(vb_gui, rect::ID);

	vb_2d = new VertexBuffer();
	create_quad(vb_2d, rect(-1,1, -1,1));


	auto blur_tex1 = new vulkan::DynamicTexture(width/2, height/2, 1, "rgba:i8");
	auto blur_tex2 = new vulkan::DynamicTexture(width/2, height/2, 1, "rgba:i8");
	auto blur_depth = new DepthBuffer(width/2, height/2, "d:f32", true);
	blur_tex1->set_options("wrap=clamp");
	blur_tex2->set_options("wrap=clamp");

	blur_render_pass = new vulkan::RenderPass({blur_tex1, blur_depth}, "clear");
	shader_blur = ResourceManager::load_shader("forward/blur.shader");
	blur_pipeline = new vulkan::Pipeline(shader_blur.get(), blur_render_pass, 0, 1);
	blur_ubo[0] = new UniformBuffer(sizeof(UBOBlur));
	blur_ubo[1] = new UniformBuffer(sizeof(UBOBlur));
	blur_dset[0] = renderer->pool->create_set(shader_blur.get());
	blur_dset[1] = renderer->pool->create_set(shader_blur.get());
	fb_small1 = new vulkan::FrameBuffer(blur_render_pass, {blur_tex1, blur_depth});
	fb_small2 = new vulkan::FrameBuffer(blur_render_pass, {blur_tex2, blur_depth});


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



void RenderPathVulkan::process_blur(CommandBuffer *cb, FrameBuffer *source, FrameBuffer *target, float threshold, int iaxis) {
	const vec2 AXIS[2] = {{2,0}, {0,1}};
	UBOBlur u;
	u.radius = cam->bloom_radius * resolution_scale_x;
	u.threshold = 0.1f;//threshold / cam->exposure;
	u.axis = AXIS[iaxis];
	blur_ubo[iaxis]->update(&u);
	blur_dset[iaxis]->set_buffer(0, blur_ubo[iaxis]);
	blur_dset[iaxis]->set_texture(1, source->attachments[0].get());
	blur_dset[iaxis]->update();

	auto rp = blur_render_pass;

	cb->begin_render_pass(rp, target);
	cb->set_viewport(rect(0,target->width, 0,target->height));

	cb->bind_pipeline(blur_pipeline);
	cb->bind_descriptor_set(0, blur_dset[iaxis]);

	cb->draw(vb_2d);

	cb->end_render_pass();

	//process(cb, {source->attachments[0].get()}, target, shader_blur.get());
}

void RenderPathVulkan::process_depth(FrameBuffer *source, FrameBuffer *target, const complex &axis) {
	/*shader_depth->set_float("max_radius", 50);
	shader_depth->set_float("focal_length", cam->focal_length);
	shader_depth->set_float("focal_blur", cam->focal_blur);
	shader_depth->set_floats("axis", &axis.x, 2);
	shader_depth->set_matrix("invproj", cam->m_projection.inverse());
	process({source->color_attachments[0].get(), depth_buffer}, target, shader_depth.get());*/
}

void RenderPathVulkan::process(CommandBuffer *cb, const Array<Texture*> &source, FrameBuffer *target, Shader *shader) {
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
				dset_gui.add(renderer->pool->create_set("buffer,sampler"));
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
		p = get_pipeline_alpha(s, renderer->default_render_pass(), m->alpha.source, m->alpha.destination);
		//msg_write(format("a %d %d  %s  %s", (int)m->alpha.source, (int)m->alpha.destination, p2s(s), p2s(p)));
	} else if (m->alpha.mode == TransparencyMode::COLOR_KEY_HARD) {
		msg_write("HARD");
		p = get_pipeline_alpha(s, renderer->default_render_pass(), Alpha::SOURCE_ALPHA, Alpha::SOURCE_INV_ALPHA);
	} else {
		p = get_pipeline(s, renderer->default_render_pass());
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
		dset->set_texture(i0 + k, tex_white);
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
				sb_dsets.add(renderer->pool->create_set(sb->material[i]->get_shader((int)type-1, ShaderVariant::DEFAULT)));
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
			tr_dsets.add(renderer->pool->create_set(t->material->get_shader((int)type-1, ShaderVariant::DEFAULT)));
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
			ob_dsets.add(renderer->pool->create_set(s.material->get_shader((int)type-1, ShaderVariant::DEFAULT)));
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

