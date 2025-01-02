/*
 * HDRRendererGL.cpp
 *
 *  Created on: 23 Nov 2021
 *      Author: michi
 */

#include "HDRRenderer.h"

#include "ThroughShaderRenderer.h"
#include "MultisampleResolver.h"
#ifdef USING_OPENGL
#include <renderer/target/TextureRendererGL.h>
#else
#include <renderer/target/TextureRendererVulkan.h>
#endif
#include "../base.h"
#include "../helper/LightMeter.h"
#include "../../graphics-impl.h"
#include <lib/math/vec2.h>
#include <lib/math/rect.h>
#include <lib/os/msg.h>
#include <lib/any/any.h>
#include "../../helper/PerformanceMonitor.h"
#include "../../helper/ResourceManager.h"
#include "../../Config.h"
#include "../../y/EngineData.h"
#include "../../world/Camera.h"

#ifdef USING_OPENGL

static float resolution_scale_x = 1.0f;
static float resolution_scale_y = 1.0f;

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
		bl.tex_out = new Texture(bloomw, bloomh, "rgba:f16");
		bl.tex_temp->set_options("wrap=clamp");
		bl.tex_out->set_options("wrap=clamp");
		bl.tsr[0] = new ThroughShaderRenderer({bloom_input}, shader_blur);
		bl.tsr[1] = new ThroughShaderRenderer({bl.tex_temp}, shader_blur);
		bl.tsr[0]->data.dict_set("axis:0", axis_x);
		bl.tsr[1]->data.dict_set("axis:0", axis_y);
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

	light_meter = new LightMeter(resource_manager, tex.get());
}

HDRRenderer::~HDRRenderer() = default;

void HDRRenderer::prepare(const RenderParams& params) {
	PerformanceMonitor::begin(ch_prepare);

	if (!cam)
		cam = cam_main;

	for (auto c: children)
		c->prepare(params);
	// FIXME
	texture_renderer->prepare(params);

	auto scaled_params = params.with_area(dynamicly_scaled_area(texture_renderer->frame_buffer.get()));
	texture_renderer->render(scaled_params);
	if (ms_resolver)
		ms_resolver->render(scaled_params);

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
	//glGenerateTextureMipmap(fb_small2->color_attachments[0]->texture);

	PerformanceMonitor::end(ch_prepare);
}

void HDRRenderer::draw(const RenderParams& params) {
	Any data;
	data.dict_set("exposure:0", cam->exposure);
	data.dict_set("bloom_factor:4", cam->bloom_factor);
	data.dict_set("scale_x:8", resolution_scale_x);
	data.dict_set("scale_y:12", resolution_scale_y);

	out_renderer->data = data;
	out_renderer->draw(params);
}

#endif



