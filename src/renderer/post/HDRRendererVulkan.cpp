/*
 * HDRRendererVulkan.cpp
 *
 *  Created on: 23 Nov 2021
 *      Author: michi
 */

#include "HDRRenderer.h"
#ifdef USING_VULKAN
#include "ThroughShaderRenderer.h"
#include "../base.h"
#include "../target/TextureRendererVulkan.h"
#include <graphics-impl.h>
#include <Config.h>
#include <helper/PerformanceMonitor.h>
#include <helper/ResourceManager.h>
#include <world/Camera.h>

void apply_shader_data(CommandBuffer* cb, const Any &shader_data);


static float resolution_scale_x = 1.0f;
static float resolution_scale_y = 1.0f;

static int BLUR_SCALE = 4;
static int BLOOM_LEVEL_SCALE = 4;


HDRRenderer::HDRRenderer(Camera *_cam, const shared<Texture>& tex, const shared<DepthBuffer>& depth_buffer) : Renderer("hdr") {
	cam = _cam;

	tex_main = tex;
	_depth_buffer = depth_buffer;

	int width = tex->width;
	int height = tex->height;


	shader_blur = resource_manager->load_shader("forward/blur.shader");
	int bloomw = width, bloomh = height;
	auto bloom_input = tex;
	Any axis_x, axis_y;
	axis_x.list_set(0, 1.0f);
	axis_x.list_set(1, 0.0f);
	axis_y.list_set(0, 0.0f);
	axis_y.list_set(1, 1.0f);
	for (int i=0; i<MAX_BLOOM_LEVELS; i++) {
		auto& bl = bloom_levels[i];
		bloomw /= BLOOM_LEVEL_SCALE;
		bloomh /= BLOOM_LEVEL_SCALE;
		bl.tex_temp = new Texture(bloomw, bloomh, "rgba:f16");
		auto depth0 = new DepthBuffer(bloomw, bloomh, "d:f32");
		bl.tex_out = new Texture(bloomw, bloomh, "rgba:f16");
		auto depth1 = new DepthBuffer(bloomw, bloomh, "d:f32");
		bl.tex_temp->set_options("wrap=clamp");
		bl.tex_out->set_options("wrap=clamp");
		bl.tsr[0] = new ThroughShaderRenderer("blur", {bloom_input}, shader_blur);
		bl.tsr[1] = new ThroughShaderRenderer("blur", {bl.tex_temp}, shader_blur);
		bl.tsr[0]->data.dict_set("axis:0", axis_x);
		bl.tsr[1]->data.dict_set("axis:0", axis_y);
		bl.renderer[0] = new TextureRenderer("blur", {bl.tex_temp, depth0});
		bl.renderer[1] = new TextureRenderer("blur", {bl.tex_out, depth1});
		bl.renderer[0]->use_params_area = true;
		bl.renderer[1]->use_params_area = true;
		bl.renderer[0]->add_child(bl.tsr[0].get());
		bl.renderer[1]->add_child(bl.tsr[1].get());
		bloom_input = bl.tex_out;
	}


	shader_out = resource_manager->load_shader("forward/hdr.shader");
	out_renderer = new ThroughShaderRenderer("out", {tex.get(), bloom_levels[0].tex_out, bloom_levels[1].tex_out, bloom_levels[2].tex_out, bloom_levels[3].tex_out}, shader_out);
}

HDRRenderer::~HDRRenderer() = default;

void HDRRenderer::prepare(const RenderParams& params) {
	auto cb = params.command_buffer;
	PerformanceMonitor::begin(ch_prepare);
	gpu_timestamp_begin(params, ch_prepare);

	if (!cam)
		cam = cam_main;

	for (auto c: children)
		c->prepare(params);

	out_renderer->set_source(dynamicly_scaled_source());


	//float r = cam->bloom_radius * engine.resolution_scale_x;
	float r = 3;//max(5 * engine.resolution_scale_x, 2.0f);
	float threshold = 1.0f;

	for (int i=0; i<MAX_BLOOM_LEVELS; i++) {
		auto& bl = bloom_levels[i];

		bl.tsr[0]->data.dict_set("radius:8", r * BLOOM_LEVEL_SCALE);
		bl.tsr[0]->data.dict_set("threshold:12", threshold);
		bl.tsr[0]->set_source(dynamicly_scaled_source());
		bl.renderer[0]->render(params.with_area(dynamicly_scaled_area(bl.renderer[0]->frame_buffer.get())));

		bl.tsr[1]->data.dict_set("radius:8", r);
		bl.tsr[1]->data.dict_set("threshold:12", 0.0f);
		bl.tsr[1]->set_source(dynamicly_scaled_source());
		bl.renderer[1]->render(params.with_area(dynamicly_scaled_area(bl.renderer[1]->frame_buffer.get())));

		r = 3;//max(5 * engine.resolution_scale_x, 3.0f);
		threshold = 0;
	}

	gpu_timestamp_end(params, ch_prepare);
	PerformanceMonitor::end(ch_prepare);

}

void HDRRenderer::draw(const RenderParams& params) {
	PerformanceMonitor::begin(channel);
	gpu_timestamp_begin(params, channel);
	Any data;
	Any a;
	auto m = mat4::ID;
	for (int i=0; i<16; i++)
		a.list_set(i, ((float*)&m)[i]);
	data.dict_set("project:128", a);
	data.dict_set("exposure:192", cam->exposure);
	data.dict_set("bloom_factor:196", cam->bloom_factor);
	data.dict_set("gamma:200", 2.2f);
	data.dict_set("scale_x:204", resolution_scale_x);
	data.dict_set("scale_y:208", resolution_scale_y);

	out_renderer->data = data;
	out_renderer->draw(params);
	gpu_timestamp_end(params, channel);
	PerformanceMonitor::end(channel);
}

#if 0

/*auto bloom_input = texture_renderer->frame_buffer.get();
float threshold = 1.0f;
for (int i=0; i<MAX_BLOOM_LEVELS; i++) {
	process_blur(cb, bloom_input, bloom_levels[i].fb_temp.get(), threshold, i*2);
	process_blur(cb, bloom_levels[i].fb_temp.get(), bloom_levels[i].fb_out.get(), 0.0f, i*2+1);
	bloom_input = bloom_levels[i].fb_out.get();
	threshold = 0;
}*/

void HDRRenderer::process_blur(CommandBuffer *cb, FrameBuffer *source, FrameBuffer *target, float threshold, int iaxis) {
	const vec2 AXIS[2] = {{1,0}, {0,1}};
	//const float SCALE[2] = {(float)BLUR_SCALE, 1};
	//UBOBlur u;
	float radius = (iaxis <= 1) ? 5 : 11;//cam->bloom_radius * resolution_scale_x * 4 / (float)BLUR_SCALE;
	//u.threshold = threshold / cam->exposure;
	//u.axis = AXIS[iaxis % 2];
	//blur_ubo[iaxis]->update(&u);
	//blur_dset[iaxis]->set_uniform_buffer(0, blur_ubo[iaxis]);
	blur_dset[iaxis]->set_texture(1, source->attachments[0].get());
	blur_dset[iaxis]->update();

	auto rp = blur_render_pass[iaxis];

	cb->begin_render_pass(rp, target);
	cb->set_viewport(dynamicly_scaled_area(target));

	cb->bind_pipeline(blur_pipeline[iaxis / 2]);
	cb->bind_descriptor_set(0, blur_dset[iaxis]);

	Any axis_x, axis_y;
	axis_x.list_set(0, 1.0f);
	axis_x.list_set(1, 0.0f);
	axis_y.list_set(0, 0.0f);
	axis_y.list_set(1, 1.0f);

	Any data;
	data.dict_set("radius:8", radius);
	data.dict_set("threshold:12", threshold / cam->exposure);
	if ((iaxis % 2) == 0)
		data.dict_set("axis:0", axis_x);
	else
		data.dict_set("axis:0", axis_y);

	apply_shader_data(cb, data);

	cb->draw(vb_2d.get());

	cb->end_render_pass();

	//process(cb, {source->attachments[0].get()}, target, shader_blur.get());
}
#endif

#endif
