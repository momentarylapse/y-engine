/*
 * HDRRendererGL.cpp
 *
 *  Created on: 23 Nov 2021
 *      Author: michi
 */

#include "HDRRendererGL.h"

#include <lib/image/image.h>
#include <renderer/target/TextureRendererGL.h>
#ifdef USING_OPENGL
#include "ThroughShaderRenderer.h"
#include "MultisampleResolver.h"
#include "../base.h"
#include "../helper/ComputeTask.h"
#include <lib/nix/nix.h>
#include <lib/math/vec2.h>
#include <lib/math/rect.h>
#include <lib/os/msg.h>
#include <lib/any/any.h>
#include "../../helper/PerformanceMonitor.h"
#include "../../helper/ResourceManager.h"
#include "../../Config.h"
#include "../../y/EngineData.h"
#include "../../world/Camera.h"

static float resolution_scale_x = 1.0f;
static float resolution_scale_y = 1.0f;

static int BLOOM_LEVEL_SCALE = 4;



HDRRendererGL::HDRRendererGL(Camera *_cam, const shared<Texture>& tex, const shared<DepthBuffer>& depth_buffer) : PostProcessorStage("hdr") {
	ch_post_blur = PerformanceMonitor::create_channel("blur", channel);
	ch_out = PerformanceMonitor::create_channel("out", channel);

	cam = _cam;
	tex_main = tex;
	_depth_buffer = depth_buffer;
	int width = tex->width;
	int height = tex->height;

	if (config.antialiasing_method == AntialiasingMethod::MSAA) {
		msg_error("yes msaa");

		auto tex_ms = new nix::TextureMultiSample(width, height, 4, "rgba:f16");
		auto depth_ms = new nix::RenderBuffer(width, height, 4, "d24s8");
		texture_renderer = new TextureRenderer({tex_ms, depth_ms});

		ms_resolver = new MultisampleResolver(tex_ms, depth_ms, tex.get(), _depth_buffer.get());
		fb_main = ms_resolver->into_texture->frame_buffer;
	} else {
		msg_error("no msaa");

		//texture_renderer = new TextureRenderer({tex.get(), _depth_buffer.get()});
		//fb_main = texture_renderer->frame_buffer;
	}

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
		bl.tex_out = new Texture(bloomw, bloomh, "rgba:f16");
		bl.tex_temp->set_options("wrap=clamp");
		bl.tex_out->set_options("wrap=clamp");
		bl.tsr[0] = new ThroughShaderRenderer({bloom_input}, shader_blur);
		bl.tsr[1] = new ThroughShaderRenderer({bl.tex_temp}, shader_blur);
		bl.tsr[0]->data.dict_set("axis", axis_x);
		bl.tsr[1]->data.dict_set("axis", axis_y);
		bl.renderer[0] = new TextureRenderer({bl.tex_temp});
		bl.renderer[1] = new TextureRenderer({bl.tex_out});
		bl.renderer[0]->use_params_area = true;
		bl.renderer[1]->use_params_area = true;
		bl.renderer[0]->add_child(bl.tsr[0].get());
		bl.renderer[1]->add_child(bl.tsr[1].get());
		bloom_input = bl.tex_out;
	}


	shader_out = resource_manager->load_shader("forward/hdr.shader");
	out_renderer = new ThroughShaderRenderer({tex.get(), bloom_levels[0].tex_out, bloom_levels[1].tex_out, bloom_levels[2].tex_out, bloom_levels[3].tex_out}, shader_out);

	light_meter.init(resource_manager, tex.get(), channel);
}

HDRRendererGL::~HDRRendererGL() = default;

void HDRRendererGL::prepare(const RenderParams& params) {
	PerformanceMonitor::begin(ch_prepare);

	if (!cam)
		cam = cam_main;

	for (auto c: children)
		c->prepare(params);

	texture_renderer->children = children;
	auto scaled_params = params.with_area(dynamicly_scaled_area(texture_renderer->frame_buffer.get()));
	texture_renderer->render(scaled_params);

	out_renderer->set_source(dynamicly_scaled_source());

	if (config.antialiasing_method == AntialiasingMethod::MSAA)
		ms_resolver->render(scaled_params);

	PerformanceMonitor::begin(ch_post_blur);
	gpu_timestamp_begin(params, ch_post_blur);
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
	//glGenerateTextureMipmap(fb_small2->color_attachments[0]->texture);
	gpu_timestamp_end(params, ch_post_blur);
	PerformanceMonitor::end(ch_post_blur);

	light_meter.measure(params, tex_main.get());
	if (cam->auto_exposure)
		light_meter.adjust_camera(cam);

	PerformanceMonitor::end(ch_prepare);
}

void HDRRendererGL::draw(const RenderParams& params) {
	Any data;
	data.dict_set("exposure", cam->exposure);
	data.dict_set("bloom_factor", cam->bloom_factor);
	data.dict_set("scale_x", resolution_scale_x);
	data.dict_set("scale_y", resolution_scale_y);

	out_renderer->data = data;
	out_renderer->draw(params);
}


void HDRRendererGL::LightMeter::init(ResourceManager* resource_manager, Texture* tex, int channel) {
	ch_post_brightness = PerformanceMonitor::create_channel("expo", channel);
	compute = new ComputeTask(resource_manager->load_shader("compute/brightness.shader"));
	params = new UniformBuffer();
	buf = new ShaderStorageBuffer();
	compute->bind_texture(0, tex);
	compute->bind_storage_buffer(1, buf);
	compute->bind_uniform_buffer(2, params);
}

void HDRRendererGL::LightMeter::measure(const RenderParams& _params, Texture* tex) {
	PerformanceMonitor::begin(ch_post_brightness);
	gpu_timestamp_begin(_params, ch_post_brightness);

	int NBINS = 256;
	histogram.resize(NBINS);
	memset(&histogram[0], 0, NBINS * sizeof(int));
	buf->update(&histogram[0], NBINS * sizeof(int));

	int pp[2] = {tex->width, tex->height};
	params->update(&pp, sizeof(pp));
	const int NSAMPLES = 256;
	compute->dispatch(NSAMPLES, 1, 1);

	buf->read(&histogram[0], NBINS*sizeof(int));
	//msg_write(str(histogram));

	gpu_timestamp_end(_params, ch_post_brightness);

	/*int s = 0;
	for (int i=0; i<NBINS; i++)
		s += histogram[i];
	msg_write(format("%d  %d", s, NSAMPLES*256));*/

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
	PerformanceMonitor::end(ch_post_brightness);
}

void HDRRendererGL::LightMeter::adjust_camera(Camera *cam) {
	float exposure = clamp(pow(1.0f / brightness, 0.8f), cam->auto_exposure_min, cam->auto_exposure_max);
	if (exposure > cam->exposure)
		cam->exposure *= 1.05f;
	if (exposure < cam->exposure)
		cam->exposure /= 1.05f;
}



#endif
