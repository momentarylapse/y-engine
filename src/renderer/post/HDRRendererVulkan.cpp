/*
 * HDRRendererVulkan.cpp
 *
 *  Created on: 23 Nov 2021
 *      Author: michi
 */

#include "HDRRendererVulkan.h"
#ifdef USING_VULKAN
#include "../base.h"
#include "../../graphics-impl.h"
#include "../../helper/PerformanceMonitor.h"
#include "../../helper/ResourceManager.h"
#include "../../lib/math/rect.h"
#include "../../lib/math/vec2.h"
#include "../../world/Camera.h"
#include "../../world/World.h"

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

static float resolution_scale_x = 1.0f;
static float resolution_scale_y = 1.0f;

static int BLUR_SCALE = 4;

HDRRendererVulkan::RenderOutData::RenderOutData(Shader *s, Renderer *r, const Array<Texture*> &tex) {
	shader_out = s;
	pipeline_out = new vulkan::Pipeline(s, r->parent->render_pass(), 0, "triangles", "3f,3f,2f");
	pipeline_out->set_culling(0);
	pipeline_out->rebuild();
	dset_out = pool->create_set("buffer,sampler,sampler");

	foreachi (auto *t, tex, i)
		dset_out->set_texture(1 + i, t);
	dset_out->update();

	vb_2d = new VertexBuffer("3f,3f,2f");
	vb_2d->create_quad(rect::ID_SYM);
}


void HDRRendererVulkan::RenderOutData::render_out(CommandBuffer *cb, const Array<float> &data) {
	vb_2d->create_quad(rect::ID_SYM, dynamicly_scaled_source());


	cb->bind_pipeline(pipeline_out);
	cb->bind_descriptor_set(0, dset_out);
	struct PCOut {
		matrix p, m, v;
		float x[32];
	};
	PCOut pco = {matrix::ID, matrix::ID, matrix::ID, cam->exposure};
	memcpy(&pco.x, &data[0], sizeof(float) * data.num);
	cb->push_constant(0, sizeof(matrix) * 3 + sizeof(float) * data.num, &pco);
	cb->draw(vb_2d);
}


HDRRendererVulkan::RenderIntoData::RenderIntoData(Renderer *r) {
	auto tex = new vulkan::DynamicTexture(r->width, r->height, 1, "rgba:f16");
	_depth_buffer = new DepthBuffer(r->width, r->height, "d:f32", true);
	_render_pass = new vulkan::RenderPass({tex, _depth_buffer}, "clear");

	fb_main = new vulkan::FrameBuffer(_render_pass, {
		tex,
		_depth_buffer});
	fb_main->attachments[0]->set_options("wrap=clamp");
}

void HDRRendererVulkan::RenderIntoData::render_into(Renderer *r) {
	if (!r)
		return;

	auto cb = r->command_buffer();

	//vb_2d->create_quad(rect::ID_SYM, dynamicly_scaled_source());

	cb->set_viewport(dynamicly_scaled_area(fb_main.get()));

	_render_pass->clear_color = {r->background()};
	cb->begin_render_pass(_render_pass, fb_main.get());

	r->draw();

	cb->end_render_pass();
}


HDRRendererVulkan::HDRRendererVulkan(Renderer *parent) : PostProcessorStage("hdr", parent) {
	ch_post_blur = PerformanceMonitor::create_channel("blur", channel);
	ch_out = PerformanceMonitor::create_channel("out", channel);

	into = RenderIntoData(this);
	fb_main = into.fb_main.get();


	auto blur_tex1 = new vulkan::DynamicTexture(width/BLUR_SCALE, height/BLUR_SCALE, 1, "rgba:f16");
	auto blur_tex2 = new vulkan::DynamicTexture(width/BLUR_SCALE, height/BLUR_SCALE, 1, "rgba:f16");
	auto blur_depth = new DepthBuffer(width/BLUR_SCALE, height/BLUR_SCALE, "d:f32", true);
	blur_tex1->set_options("wrap=clamp");
	blur_tex2->set_options("wrap=clamp");

	blur_render_pass = new vulkan::RenderPass({blur_tex1, blur_depth}, "clear");
	// without clear, we get artifacts from dynamic resolution scaling
	shader_blur = ResourceManager::load_shader("forward/blur.shader");
	blur_pipeline = new vulkan::Pipeline(shader_blur.get(), blur_render_pass, 0, "triangles", "3f,3f,2f");
	blur_pipeline->set_z(false, false);
	blur_pipeline->rebuild();
	blur_ubo[0] = new UniformBuffer(sizeof(UBOBlur));
	blur_ubo[1] = new UniformBuffer(sizeof(UBOBlur));
	blur_dset[0] = pool->create_set(shader_blur.get());
	blur_dset[1] = pool->create_set(shader_blur.get());
	fb_small1 = new vulkan::FrameBuffer(blur_render_pass, {blur_tex1, blur_depth});
	fb_small2 = new vulkan::FrameBuffer(blur_render_pass, {blur_tex2, blur_depth});

	out = RenderOutData(ResourceManager::load_shader("forward/hdr.shader"), this, {into.fb_main->attachments[0].get(), fb_small2->attachments[0].get()});



	vb_2d = new VertexBuffer("3f,3f,2f");
	vb_2d->create_quad(rect::ID_SYM);

}

HDRRendererVulkan::~HDRRendererVulkan() {
}

void HDRRendererVulkan::prepare() {
	if (child)
		child->prepare();

	vb_2d->create_quad(rect::ID_SYM, dynamicly_scaled_source());

	into.render_into(child);

	auto cb = command_buffer();

	// render blur into fb_small2!
	PerformanceMonitor::begin(ch_post_blur);
	process_blur(cb, into.fb_main.get(), fb_small1.get(), 1.0f, 0);
	process_blur(cb, fb_small1.get(), fb_small2.get(), 0.0f, 1);
	PerformanceMonitor::end(ch_post_blur);

}

void HDRRendererVulkan::draw() {
	auto cb = command_buffer();


	PerformanceMonitor::begin(ch_out);
	out.render_out(cb, {cam->exposure, cam->bloom_factor, 2.2f, resolution_scale_x, resolution_scale_y});
	PerformanceMonitor::end(ch_out);
}

void HDRRendererVulkan::process_blur(CommandBuffer *cb, FrameBuffer *source, FrameBuffer *target, float threshold, int iaxis) {
	const vec2 AXIS[2] = {{(float)BLUR_SCALE,0}, {0,1}};
	//const float SCALE[2] = {(float)BLUR_SCALE, 1};
	UBOBlur u;
	u.radius = cam->bloom_radius * resolution_scale_x * 4 / (float)BLUR_SCALE;
	u.threshold = threshold / cam->exposure;
	u.axis = AXIS[iaxis];
	blur_ubo[iaxis]->update(&u);
	blur_dset[iaxis]->set_buffer(0, blur_ubo[iaxis]);
	blur_dset[iaxis]->set_texture(1, source->attachments[0].get());
	blur_dset[iaxis]->update();

	auto rp = blur_render_pass;

	cb->begin_render_pass(rp, target);
	cb->set_viewport(dynamicly_scaled_area(target));

	cb->bind_pipeline(blur_pipeline);
	cb->bind_descriptor_set(0, blur_dset[iaxis]);

	cb->draw(vb_2d);

	cb->end_render_pass();

	//process(cb, {source->attachments[0].get()}, target, shader_blur.get());
}

#endif
