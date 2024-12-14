/*
 * HDRRendererVulkan.cpp
 *
 *  Created on: 23 Nov 2021
 *      Author: michi
 */

#include "HDRRendererVulkan.h"
#ifdef USING_VULKAN
#include "ThroughShaderRenderer.h"
#include "../base.h"
#include "../target/TextureRendererVulkan.h"
#include "../helper/ComputeTask.h"
#include <graphics-impl.h>
#include <Config.h>
#include <helper/PerformanceMonitor.h>
#include <helper/ResourceManager.h>
#include <lib/base/iter.h>
#include <lib/math/rect.h>
#include <lib/math/vec2.h>
#include <lib/os/msg.h>
#include <world/Camera.h>
#include <world/World.h>

void apply_shader_data(CommandBuffer* cb, const Any &shader_data);

/*struct UBOBlur{
	vec2 axis;
	//float dummy1[2];
	float radius;
	float threshold;
	float dummy2[2];
	float kernel[20];
};*/

//UniformBuffer *blur_ubo[HDRRendererVulkan::MAX_BLOOM_LEVELS*2];
DescriptorSet *blur_dset[HDRRendererVulkan::MAX_BLOOM_LEVELS*2];
GraphicsPipeline *blur_pipeline[HDRRendererVulkan::MAX_BLOOM_LEVELS];
RenderPass *blur_render_pass[HDRRendererVulkan::MAX_BLOOM_LEVELS*2];

static float resolution_scale_x = 1.0f;
static float resolution_scale_y = 1.0f;

static int BLUR_SCALE = 4;
static int BLOOM_LEVEL_SCALE = 4;
static int BLOOM_HEIGHT0 = 256;

HDRRendererVulkan::RenderOutData::RenderOutData(Shader *s, const Array<Texture*> &tex) {
	shader_out = s;
	dset_out = pool->create_set("buffer,sampler,sampler,sampler,sampler,sampler");

	for (auto&& [i, t]: enumerate(tex))
		dset_out->set_texture(1 + i, t);
	dset_out->update();

	vb_2d = new VertexBuffer("3f,3f,2f");
	vb_2d->create_quad(rect::ID_SYM);
	vb_2d_current_source = rect::ID_SYM;
}

void HDRRendererVulkan::RenderOutData::render_out(CommandBuffer *cb, const Array<float> &data, float exposure, const RenderParams& params) {
	auto source = dynamicly_scaled_source();
	if (source != vb_2d_current_source) {
		vb_2d->create_quad(rect::ID_SYM, source);
		vb_2d_current_source = source;
	}

	if (!pipeline_out) {
		pipeline_out = new vulkan::GraphicsPipeline(shader_out.get(), params.render_pass, 0, "triangles", "3f,3f,2f");
		pipeline_out->set_culling(CullMode::NONE);
		pipeline_out->set_z(false, false);
		pipeline_out->rebuild();
	}

	cb->bind_pipeline(pipeline_out);
	cb->bind_descriptor_set(0, dset_out);
	struct PCOut {
		mat4 p, m, v;
		float x[32];
	};
	PCOut pco = {mat4::ID, mat4::ID, mat4::ID, exposure};
	memcpy(&pco.x, &data[0], sizeof(float) * data.num);
	cb->push_constant(0, sizeof(mat4) * 3 + sizeof(float) * data.num, &pco);
	cb->draw(vb_2d);
}


HDRRendererVulkan::HDRRendererVulkan(Camera *_cam, const shared<Texture>& tex, const shared<DepthBuffer>& depth_buffer) : PostProcessorStage("hdr") {
	cam = _cam;
	ch_post_blur = PerformanceMonitor::create_channel("blur", channel);
	ch_out = PerformanceMonitor::create_channel("out", channel);

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
		bl.tsr[0] = new ThroughShaderRenderer({bloom_input}, shader_blur);
		bl.tsr[1] = new ThroughShaderRenderer({bl.tex_temp}, shader_blur);
		bl.tsr[0]->data.dict_set("axis", axis_x);
		bl.tsr[1]->data.dict_set("axis", axis_y);
		bl.renderer[0] = new TextureRenderer({bl.tex_temp, depth0});
		bl.renderer[1] = new TextureRenderer({bl.tex_out, depth1});
		bl.renderer[0]->use_params_area = true;
		bl.renderer[1]->use_params_area = true;
		bl.renderer[0]->add_child(bl.tsr[0].get());
		bl.renderer[1]->add_child(bl.tsr[1].get());
		bloom_input = bl.tex_out;

		blur_render_pass[i*2] = bl.renderer[0]->render_pass.get();
		blur_render_pass[i*2+1] = bl.renderer[1]->render_pass.get();
	//	blur_pipeline[i] = bl.tsr[0]->pipeline;
		blur_dset[i*2] = bl.tsr[0]->dset;
		blur_dset[i*2+1] = bl.tsr[1]->dset;
		bl.fb_temp = bl.renderer[0]->frame_buffer;
		bl.fb_out = bl.renderer[1]->frame_buffer;
		blur_pipeline[i] = new vulkan::GraphicsPipeline(shader_blur.get(), blur_render_pass[i*2], 0, "triangles", "3f,3f,2f");
		blur_pipeline[i]->set_z(false, false);
		blur_pipeline[i]->rebuild();
	}


	shader_out = resource_manager->load_shader("forward/hdr.shader");
	Array<Texture*> _tex = {tex.get(), bloom_levels[0].tex_out.get(), bloom_levels[1].tex_out.get(), bloom_levels[2].tex_out.get(), bloom_levels[3].tex_out.get()};
	out = RenderOutData(shader_out.get(), _tex);



	vb_2d = new VertexBuffer("3f,3f,2f");
	vb_2d->create_quad(rect::ID_SYM);

	light_meter.init(resource_manager, tex.get(), channel);
}

HDRRendererVulkan::~HDRRendererVulkan() = default;

void HDRRendererVulkan::prepare(const RenderParams& params) {
	auto cb = params.command_buffer;
	PerformanceMonitor::begin(ch_prepare);
	gpu_timestamp_begin(params, ch_prepare);

	if (!cam)
		cam = cam_main;


	for (auto c: children)
		c->prepare(params);
	texture_renderer->prepare(params);

	auto source = dynamicly_scaled_source();
	if (vb_2d_current_source != source) {
		vb_2d->create_quad(rect::ID_SYM, source);
		vb_2d_current_source = source;
	}

	auto scaled_params = params.with_area(dynamicly_scaled_area(texture_renderer->frame_buffer.get()));
	texture_renderer->render(scaled_params);

	// render blur into fb_small2!
	PerformanceMonitor::begin(ch_post_blur);
	gpu_timestamp_begin(params, ch_post_blur);
	auto bloom_input = texture_renderer->frame_buffer.get();
	float threshold = 1.0f;
	for (int i=0; i<MAX_BLOOM_LEVELS; i++) {
		process_blur(cb, bloom_input, bloom_levels[i].fb_temp.get(), threshold, i*2);
		process_blur(cb, bloom_levels[i].fb_temp.get(), bloom_levels[i].fb_out.get(), 0.0f, i*2+1);
		bloom_input = bloom_levels[i].fb_out.get();
		threshold = 0;
	}
	gpu_timestamp_end(params, ch_post_blur);
	PerformanceMonitor::end(ch_post_blur);

	light_meter.measure(params, tex_main.get());
	if (cam->auto_exposure)
		light_meter.adjust_camera(cam);

	gpu_timestamp_end(params, ch_prepare);
	PerformanceMonitor::end(ch_prepare);

}

void HDRRendererVulkan::draw(const RenderParams& params) {
	auto cb = params.command_buffer;


	PerformanceMonitor::begin(ch_out);
	gpu_timestamp_begin(params, ch_out);
	out.render_out(cb, {cam->exposure, cam->bloom_factor, 2.2f, resolution_scale_x, resolution_scale_y}, cam->exposure, params);
	gpu_timestamp_end(params, ch_out);
	PerformanceMonitor::end(ch_out);
}

void HDRRendererVulkan::process_blur(CommandBuffer *cb, FrameBuffer *source, FrameBuffer *target, float threshold, int iaxis) {
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

void HDRRendererVulkan::LightMeter::init(ResourceManager* resource_manager, Texture* tex, int channel) {
	ch_post_brightness = PerformanceMonitor::create_channel("expo", channel);
	compute = new ComputeTask(resource_manager->load_shader("compute/brightness.shader"));
	params = new UniformBuffer(8);
	buf = new ShaderStorageBuffer(256*4);
	compute->bind_texture(0, tex);
	compute->bind_storage_buffer(1, buf);
	compute->bind_uniform_buffer(2, params);
}

void HDRRendererVulkan::LightMeter::measure(const RenderParams& _params, Texture* tex) {
	PerformanceMonitor::begin(ch_post_brightness);
	gpu_timestamp_begin(_params, ch_post_brightness);
	auto cb = _params.command_buffer;

	int pp[2] = {tex->width, tex->height};
	params->update(&pp);

	int NBINS = 256;
	const int NSAMPLES = 256;
	if (histogram.num == NBINS) {
		void* p = buf->map();
		memcpy(&histogram[0], p, NBINS*sizeof(int));
		buf->unmap();
		//msg_write(str(histogram));

		int thresh = (NSAMPLES * 16 * 16) / 200 * 199;
		int n = 0;
		int ii = 0;
		for (int i=0; i<NBINS; i++) {
			n += histogram[i];
			if (n > thresh) {
				ii = i;
				break;
			}
		}
		brightness = pow(2.0f, ((float)ii / (float)NBINS) * 20.0f - 10.0f);
	}

	histogram.resize(NBINS);
	memset(&histogram[0], 0, NBINS * sizeof(int));
	buf->update(&histogram[0]);
	compute->dispatch(cb, NSAMPLES, 1, 1);

	gpu_timestamp_end(_params, ch_post_brightness);
	PerformanceMonitor::end(ch_post_brightness);
}

void HDRRendererVulkan::LightMeter::adjust_camera(Camera *cam) {
	float exposure = clamp((float)pow(1.0f / brightness, 0.8f), cam->auto_exposure_min, cam->auto_exposure_max);
	if (exposure > cam->exposure)
		cam->exposure *= 1.05f;
	if (exposure < cam->exposure)
		cam->exposure /= 1.05f;
}

#endif
