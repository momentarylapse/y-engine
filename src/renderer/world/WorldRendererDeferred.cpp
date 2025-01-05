/*
 * WorldRenderer.cpp
 *
 *  Created on: 07.08.2020
 *      Author: michi
 */

#include "WorldRendererDeferred.h"

//#ifdef USING_OPENGL
#include "geometry/GeometryRenderer.h"
#include "../target/TextureRendererVulkan.h"
#include "../target/TextureRendererGL.h"
#include "pass/ShadowRenderer.h"
#include "../post/ThroughShaderRenderer.h"
#include "../base.h"
#include "../path/RenderPath.h"
#include <lib/nix/nix.h>
#include <lib/os/msg.h>
#include <lib/math/random.h>
#include <lib/math/vec4.h>
#include <lib/math/vec2.h>

#include "../../helper/PerformanceMonitor.h"
#include "../../helper/ResourceManager.h"
#include "../../plugins/ControllerManager.h"
#include "../../plugins/PluginManager.h"
#include "../../world/Camera.h"
#include "../../world/Light.h"
#include "../../world/World.h"
#include "../../y/Entity.h"
#include "../../Config.h"
#include "../../meta.h"
#include "../../graphics-impl.h"


WorldRendererDeferred::WorldRendererDeferred(SceneView& scene_view, int width, int height) : WorldRenderer("world/def", scene_view) {

	auto tex1 = new Texture(width, height, "rgba:f16"); // diffuse
	auto tex2 = new Texture(width, height, "rgba:f16"); // emission
	auto tex3 = new Texture(width, height, "rgba:f16"); // pos
	auto tex4 = new Texture(width, height, "rgba:f16"); // normal,reflectivity
	auto depth = new DepthBuffer(width, height, "d24s8");
	gbuffer_textures = {tex1, tex2, tex3, tex4, depth};
	for (auto a: weak(gbuffer_textures))
		a->set_options("wrap=clamp,magfilter=nearest,minfilter=nearest");


	gbuffer_renderer = new TextureRenderer("gbuf", gbuffer_textures);
/*#ifdef USING_VULKAN
	render_pass = new RenderPass(weak(gbuffer_textures), {});
	gbuffer = new FrameBuffer(render_pass.get(), gbuffer_textures);
#else
	gbuffer = new FrameBuffer(gbuffer_textures);
#endif*/



	resource_manager->load_shader_module("forward/module-surface.shader");
	resource_manager->load_shader_module("deferred/module-surface.shader");

	auto shader_gbuffer_out = resource_manager->load_shader("deferred/out.shader");
//	if (!shader_gbuffer_out->link_uniform_block("SSAO", 13))
//		msg_error("SSAO");

	out_renderer = new ThroughShaderRenderer("out", shader_gbuffer_out);
	out_renderer->bind_textures(0, {tex1, tex2, tex3, tex4, depth});


	Array<vec4> ssao_samples;
	Random r;
	for (int i=0; i<64; i++) {
		auto v = r.dir() * pow(r.uniform01(), 1);
		ssao_samples.add(vec4(v.x, v.y, abs(v.z), 0));
	}
	ssao_sample_buffer = new UniformBuffer(ssao_samples.num * sizeof(vec4));
	ssao_sample_buffer->update_array(ssao_samples);

	ch_gbuf_out = PerformanceMonitor::create_channel("gbuf-out", channel);
	ch_trans = PerformanceMonitor::create_channel("trans", channel);

	geo_renderer_trans = new GeometryRenderer(RenderPathType::Forward, scene_view);
	add_child(geo_renderer_trans.get());
}

void WorldRendererDeferred::prepare(const RenderParams& params) {
	PerformanceMonitor::begin(ch_prepare);


	auto sub_params = params.with_target(gbuffer_renderer->frame_buffer.get());

	gbuffer_renderer->prepare(params);


	geo_renderer->prepare(sub_params);
	geo_renderer_trans->prepare(params); // keep drawing into direct target


	render_into_gbuffer(gbuffer_renderer->frame_buffer.get(), sub_params);

	//auto source = do_post_processing(fb_main.get());

	PerformanceMonitor::end(ch_prepare);
}

void WorldRendererDeferred::draw(const RenderParams& params) {
	PerformanceMonitor::begin(channel);
	gpu_timestamp_begin(params, channel);

	draw_background(params);

	render_out_from_gbuffer(gbuffer_renderer->frame_buffer.get(), params);

#ifdef USING_OPENGL
	auto& rvd = geo_renderer_trans->cur_rvd;

	PerformanceMonitor::begin(ch_trans);
	bool flip_y = params.target_is_window;
	mat4 m = flip_y ? mat4::scale(1,-1,1) : mat4::ID;
	auto cam = scene_view.cam;
	cam->update_matrices(params.desired_aspect_ratio);
	nix::set_projection_matrix(m * cam->m_projection);
	nix::bind_uniform_buffer(1, rvd.ubo_light.get());
	nix::set_view_matrix(cam->view_matrix());
	nix::set_z(true, true);
	nix::set_front(flip_y ? nix::Orientation::CW : nix::Orientation::CCW);

	geo_renderer_trans->set(GeometryRenderer::Flags::ALLOW_TRANSPARENT);
	geo_renderer_trans->draw(params);
	nix::set_cull(nix::CullMode::BACK);
	nix::set_front(nix::Orientation::CW);

	nix::set_z(false, false);
	nix::set_projection_matrix(mat4::ID);
	nix::set_view_matrix(mat4::ID);
	PerformanceMonitor::end(ch_trans);
#endif

	gpu_timestamp_end(params, channel);
	PerformanceMonitor::end(channel);
}

void WorldRendererDeferred::draw_background(const RenderParams& params) {
	PerformanceMonitor::begin(ch_bg);

	//nix::clear_color(Green);
	auto fb = params.frame_buffer;
#ifdef USING_VULKAN
#else
	nix::clear_color(world.background);
#endif

	auto& rvd = geo_renderer->cur_rvd;
	
	auto cam = scene_view.cam;
//	float max_depth = cam->max_depth;
	cam->max_depth = 2000000;
	bool flip_y = params.target_is_window;
	mat4 m = flip_y ? mat4::scale(1,-1,1) : mat4::ID;
	cam->update_matrices(params.desired_aspect_ratio);
	rvd.set_projection_matrix(m * cam->m_projection);

	geo_renderer->set(GeometryRenderer::Flags::ALLOW_SKYBOXES);
	geo_renderer->draw(params);
	PerformanceMonitor::end(ch_bg);

}

void WorldRendererDeferred::render_out_from_gbuffer(FrameBuffer *source, const RenderParams& params) {
	PerformanceMonitor::begin(ch_gbuf_out);

	auto& data = out_renderer->bindings.shader_data;

	if (geo_renderer->using_view_space)
		data.dict_set("eye_pos", vec3_to_any(vec3::ZERO));
	else
		data.dict_set("eye_pos", vec3_to_any(scene_view.cam->owner->pos)); // NAH
	data.dict_set("num_lights", scene_view.lights.num);
	data.dict_set("shadow_index", scene_view.shadow_index);
	data.dict_set("ambient_occlusion_radius", config.ambient_occlusion_radius);
	out_renderer->bind_uniform_buffer(13, ssao_sample_buffer);

	auto& rvd = geo_renderer->cur_rvd;
	out_renderer->bind_uniform_buffer(1, rvd.ubo_light.get());
	auto tex = weak(gbuffer_textures);
	tex.add(scene_view.shadow_maps[0]);
	tex.add(scene_view.shadow_maps[1]);
	for (int i=0; i<tex.num; i++)
		out_renderer->bind_texture(i, tex[i]);

	float resolution_scale_x = 1.0f;
	data.dict_set("resolution_scale", vec2_to_any(vec2(resolution_scale_x, resolution_scale_x)));

	out_renderer->set_source(dynamicly_scaled_source());
	out_renderer->draw(params);

	// ...
	//geo_renderer->draw_transparent();

	PerformanceMonitor::end(ch_gbuf_out);
}

//void WorldRendererDeferred::render_into_texture(nix::FrameBuffer *fb, Camera *cam) {}

void WorldRendererDeferred::render_into_gbuffer(FrameBuffer *fb, const RenderParams& params) {
	PerformanceMonitor::begin(ch_world);
	gpu_timestamp_begin(params, ch_world);


//	gbuffer_renderer->add_child(geo_renderer);
//	gbuffer_renderer->render(params);


#ifdef USING_OPENGL

	nix::bind_frame_buffer(fb);
	nix::set_viewport(dynamicly_scaled_area(fb));

	//nix::clear_color(Green);//world.background);
	nix::clear_z();
	//fb->clear_color(2, color(0, 0,0,max_depth * 0.99f));
	fb->clear_color(0, color(-1, 0,1,0));


	auto& rvd = geo_renderer->cur_rvd;
//	rvd.begin_scene(&scene_view);
	auto cam = scene_view.cam;
	cam->update_matrices(params.desired_aspect_ratio);
	nix::set_projection_matrix(mat4::scale(1,1,1) * cam->m_projection);

	nix::set_cull(nix::CullMode::BACK);
	nix::set_front(nix::Orientation::CCW);

	geo_renderer->set(GeometryRenderer::Flags::ALLOW_OPAQUE);
	geo_renderer->draw(params);

	/*nix::set_cull(nix::CullMode::BACK);
	nix::set_front(nix::Orientation::CCW);
	geo_renderer->set(GeometryRenderer::Flags::ALLOW_FX);
	geo_renderer->draw(params);*/

#endif

	gpu_timestamp_end(params, ch_world);
	PerformanceMonitor::end(ch_world);
}

