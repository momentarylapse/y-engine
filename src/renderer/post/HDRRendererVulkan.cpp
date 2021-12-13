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


HDRRendererVulkan::HDRRendererVulkan(Renderer *parent) : PostProcessorStage("hdr", parent) {
	ch_post_blur = PerformanceMonitor::create_channel("blur", channel);
	ch_out = PerformanceMonitor::create_channel("out", channel);

	auto tex = new vulkan::DynamicTexture(width, height, 1, "rgba:f16");
	_depth_buffer = new DepthBuffer(width, height, "d:f32", true);
	_render_pass = new vulkan::RenderPass({tex, _depth_buffer}, "clear");

	fb_main = new vulkan::FrameBuffer(_render_pass, {
		tex,
		_depth_buffer});
	fb_main->attachments[0]->set_options("wrap=clamp");


	auto blur_tex1 = new vulkan::DynamicTexture(width/2, height/2, 1, "rgba:f16");
	auto blur_tex2 = new vulkan::DynamicTexture(width/2, height/2, 1, "rgba:f16");
	auto blur_depth = new DepthBuffer(width/2, height/2, "d:f32", true);
	blur_tex1->set_options("wrap=clamp");
	blur_tex2->set_options("wrap=clamp");

	blur_render_pass = new vulkan::RenderPass({blur_tex1, blur_depth}, "clear");
	shader_blur = ResourceManager::load_shader("forward/blur.shader");
	blur_pipeline = new vulkan::Pipeline(shader_blur.get(), blur_render_pass, 0, "3f,3f,2f");
	blur_ubo[0] = new UniformBuffer(sizeof(UBOBlur));
	blur_ubo[1] = new UniformBuffer(sizeof(UBOBlur));
	blur_dset[0] = pool->create_set(shader_blur.get());
	blur_dset[1] = pool->create_set(shader_blur.get());
	fb_small1 = new vulkan::FrameBuffer(blur_render_pass, {blur_tex1, blur_depth});
	fb_small2 = new vulkan::FrameBuffer(blur_render_pass, {blur_tex2, blur_depth});

	shader_out = ResourceManager::load_shader("forward/hdr.shader");
	pipeline_out = new vulkan::Pipeline(shader_out.get(), parent->render_pass(), 0, "3f,3f,2f");
	pipeline_out->set_culling(0);
	pipeline_out->rebuild();
	dset_out = pool->create_set("buffer,sampler,sampler");



	vb_2d = new VertexBuffer("3f,3f,2f");
	vb_2d->create_quad(rect::ID_SYM);

}

HDRRendererVulkan::~HDRRendererVulkan() {
}

void HDRRendererVulkan::prepare() {
	if (child)
		child->prepare();


	auto cb = command_buffer();

	vb_2d->create_quad(rect::ID_SYM, dynamicly_scaled_source());

	// into fb_main
	auto cur = fb_main.get();
	cb->set_viewport(dynamicly_scaled_area(cur));

	_render_pass->clear_color = {world.background};
	cb->begin_render_pass(_render_pass, cur);

	if (child)
		child->draw();

	cb->end_render_pass();


	// render blur into fb3!
	PerformanceMonitor::begin(ch_post_blur);
	process_blur(cb, cur, fb_small1.get(), 1.0f, 0);
	process_blur(cb, fb_small1.get(), fb_small2.get(), 0.0f, 1);
	PerformanceMonitor::end(ch_post_blur);

}

void HDRRendererVulkan::draw() {
	auto cb = command_buffer();


	render_out(cb, fb_main.get(), fb_small2->attachments[0].get());
}

void HDRRendererVulkan::process_blur(CommandBuffer *cb, FrameBuffer *source, FrameBuffer *target, float threshold, int iaxis) {
	const vec2 AXIS[2] = {{2,0}, {0,1}};
	UBOBlur u;
	u.radius = cam->bloom_radius * resolution_scale_x;
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

void HDRRendererVulkan::render_out(CommandBuffer *cb, FrameBuffer *source, Texture *bloom) {
	PerformanceMonitor::begin(ch_out);

	cb->bind_pipeline(pipeline_out);
	dset_out->set_texture(1, source->attachments[0].get());
	dset_out->set_texture(2, bloom);
	dset_out->update();
	cb->bind_descriptor_set(0, dset_out);
	struct PCOut {
		matrix p, m, v;
		float exposure;
		float bloom_factor;
		float gamma;
		float scale_x;
		float scale_y;
	};
	PCOut pco = {matrix::ID, matrix::ID, matrix::ID, cam->exposure, cam->bloom_factor, 2.2f, resolution_scale_x, resolution_scale_y};
	cb->push_constant(0, sizeof(pco), &pco);
	cb->draw(vb_2d);

	PerformanceMonitor::end(ch_out);
}

#endif
