//
// Created by michi on 1/3/25.
//

#include "RenderPath.h"
#include "RenderPathDirect.h"
#include "../base.h"
#include "../world/WorldRenderer.h"
#include "../world/geometry/GeometryRendererGL.h"
#include "../world/geometry/GeometryRendererVulkan.h"
#include "../world/pass/ShadowRenderer.h"
#include "../post/ThroughShaderRenderer.h"
#include "../post/MultisampleResolver.h"
#include "../regions/RegionRenderer.h"
#include "../post/HDRResolver.h"
#include "../world/WorldRenderer.h"
#include "../world/WorldRendererForward.h"
#ifdef USING_VULKAN
	#include "../world/WorldRendererVulkanRayTracing.h"
#else
	#include "../world/WorldRendererGLDeferred.h"
#endif
#include "../helper/LightMeter.h"
#include <renderer/target/TextureRendererGL.h>
#include <renderer/target/TextureRendererVulkan.h>
#include <renderer/helper/CubeMapSource.h>
#include "../../helper/ResourceManager.h"
#include <graphics-impl.h>
#include <world/Camera.h>
#include <world/Model.h>
#include <world/World.h>
#include <y/EngineData.h>
#include <y/Entity.h>
#include <y/ComponentManager.h>
#include <Config.h>
#include <lib/os/msg.h>


HDRResolver *create_hdr_resolver(Camera *cam, Texture* tex, DepthBuffer* depth) {
	return new HDRResolver(cam, tex, depth);
}

WorldRenderer *create_world_renderer(Camera *cam, SceneView& scene_view, RenderViewData& main_rvd, RenderPathType type) {
#ifdef USING_VULKAN
	if (type == RenderPathType::PathTracing)
		return new WorldRendererVulkanRayTracing(device, cam, scene_view, engine.width, engine.height);
#else
	if (type == RenderPathType::Deferred)
		return new WorldRendererGLDeferred(cam, scene_view, engine.width, engine.height);
#endif
	return new WorldRendererForward(cam, scene_view, main_rvd);
}


RenderPath::RenderPath(RenderPathType _type, Camera* _cam) : Renderer("path") {
	type = _type;
	shadow_box_size = config.get_float("shadow.boxsize", 2000);
	shadow_resolution = config.get_int("shadow.resolution", 1024);

	scene_view.cam = _cam;

	resource_manager->default_shader = "default.shader";
	resource_manager->load_shader_module("module-basic-interface.shader");
	resource_manager->load_shader_module("module-basic-data.shader");
	if (config.get_str("renderer.shader-quality", "pbr") == "pbr") {
		resource_manager->load_shader_module("module-lighting-pbr.shader");
	} else {
		resource_manager->load_shader_module("module-lighting-simple.shader");
	}
	resource_manager->load_shader_module("module-vertex-default.shader");
	resource_manager->load_shader_module("module-vertex-animated.shader");
	resource_manager->load_shader_module("module-vertex-instanced.shader");
	resource_manager->load_shader_module("module-vertex-lines.shader");
	resource_manager->load_shader_module("module-vertex-points.shader");
	resource_manager->load_shader_module("module-vertex-fx.shader");
	resource_manager->load_shader_module("module-geometry-lines.shader");
	resource_manager->load_shader_module("module-geometry-points.shader");


	// not sure this is a good idea...
	auto e = new Entity;
	cube_map_source = new CubeMapSource;
	cube_map_source->owner = e;
	cube_map_source->cube_map = new CubeMap(cube_map_source->resolution, "rgba:i8");

	scene_view.cube_map = cube_map_source->cube_map;
}

RenderPath::~RenderPath() = default;

void RenderPath::prepare_basics() {
	if (!scene_view.cam)
		scene_view.cam = cam_main;

	scene_view.check_terrains(cam_main->owner->pos);
}

void RenderPath::create_shadow_renderer() {
	shadow_renderer = new ShadowRenderer();
	scene_view.fb_shadow1 = shadow_renderer->cascades[0].fb;
	scene_view.fb_shadow2 = shadow_renderer->cascades[1].fb;
	add_child(shadow_renderer.get());
}

void RenderPath::create_geometry_renderer() {
	geo_renderer = new GeometryRenderer(type == RenderPathType::Deferred ? RenderPathType::Deferred : RenderPathType::Forward, scene_view);
	add_child(geo_renderer.get());
}

void RenderPath::prepare_lights(Camera *cam, RenderViewData &rvd) {
	//PerformanceMonitor::begin(ch_prepare_lights);
	scene_view.prepare_lights(shadow_box_size, rvd.ubo_light.get());
#ifdef USING_VULKAN
	rvd.ubo_light->update_part(&scene_view.lights[0], 0, scene_view.lights.num * sizeof(scene_view.lights[0]));
#endif
	//PerformanceMonitor::end(ch_prepare_lights);
}



void RenderPath::render_into_cubemap(CubeMapSource& source) {
	if (!source.depth_buffer)
		source.depth_buffer = new DepthBuffer(source.resolution, source.resolution, "ds:u24i8");
	if (!source.cube_map)
		source.cube_map = new CubeMap(source.resolution, "rgba:i8");
#ifdef USING_VULKAN
	if (!source.render_pass)
		source.render_pass = new vulkan::RenderPass({source.cube_map.get(), source.depth_buffer.get()}, {"autoclear"});
#endif
	if (!source.frame_buffer[0])
		for (int i=0; i<6; i++) {
#ifdef USING_VULKAN
			source.frame_buffer[i] = new FrameBuffer(source.render_pass.get(), {source.cube_map.get(), source.depth_buffer.get()});
			try {
				source.frame_buffer[i]->update_x(source.render_pass.get(), {source.cube_map.get(), source.depth_buffer.get()}, i);
			} catch(Exception &e) {
				msg_error(e.message());
				return;
			}
#else
			source.frame_buffer[i] = new FrameBuffer();
			try {
				source.frame_buffer[i]->update_x({source.cube_map.get(), source.depth_buffer.get()}, i);
			} catch(Exception &e) {
				msg_error(e.message());
				return;
			}
#endif
		}
	Entity o(source.owner->pos, quaternion::ID);
	Camera cam;
	cam.min_depth = source.min_depth;
	cam.owner = &o;
	cam.fov = pi/2;
	for (int i=0; i<6; i++) {
		if (i == 0)
			o.ang = quaternion::rotation(vec3(0,pi/2,0));
		if (i == 1)
			o.ang = quaternion::rotation(vec3(0,-pi/2,0));
		if (i == 2)
			o.ang = quaternion::rotation(vec3(-pi/2,pi,pi));
		if (i == 3)
			o.ang = quaternion::rotation(vec3(pi/2,pi,pi));
		if (i == 4)
			o.ang = quaternion::rotation(vec3(0,0,0));
		if (i == 5)
			o.ang = quaternion::rotation(vec3(0,pi,0));
		//prepare_lights(&cam);
		render_into_texture(source.frame_buffer[i].get(), &cam, source.rvd[i]);
	}
	cam.owner = nullptr;
}


void RenderPath::suggest_cube_map_pos() {
	if (!cube_map_source)
		return;
	if (world.ego) {
		cube_map_source->owner->pos = world.ego->pos;
		cube_map_source->min_depth = 200;
		if (auto m = world.ego->get_component<Model>())
			cube_map_source->min_depth = m->prop.radius * 1.1f;
		return;
	}
	if (!scene_view.cam)
		return;
	auto& list = ComponentManager::get_list_family<Model>();
	float max_score = 0;
	cube_map_source->owner->pos = scene_view.cam->m_view * vec3(0,0,1000);
	cube_map_source->min_depth = 1000;
	for (auto m: list)
		for (auto mat: m->material) {
			float score = mat->metal;
			if (score > max_score) {
				max_score = score;
				cube_map_source->owner->pos = m->owner->pos;
				cube_map_source->min_depth = m->prop.radius;
			}
		}
}

void RenderPath::render_cubemaps(const RenderParams &params) {
	suggest_cube_map_pos();
	auto cube_map_sources = ComponentManager::get_list<CubeMapSource>();
	cube_map_sources.add(cube_map_source);
	for (auto& source: cube_map_sources) {
		if (source->update_rate <= 0)
			continue;
		source->counter ++;
		if (source->counter >= source->update_rate) {
			render_into_cubemap(*source);
			source->counter = 0;
		}
	}
}



class RenderPathComplex : public RenderPath {
public:
	explicit RenderPathComplex(Camera* cam, RenderPathType type) : RenderPath(type, cam) {

		world_renderer = create_world_renderer(cam, scene_view, main_rvd, type);
		create_shadow_renderer();
		create_geometry_renderer();
		world_renderer->geo_renderer = geo_renderer.get();

		auto hdr_tex = new Texture(engine.width, engine.height, "rgba:f16");
		hdr_tex->set_options("wrap=clamp,minfilter=nearest");
		hdr_tex->set_options("magfilter=" + config.resolution_scale_filter);
		auto hdr_depth = new DepthBuffer(engine.width, engine.height, "d:f32");

		hdr_resolver = create_hdr_resolver(cam, hdr_tex, hdr_depth);

#ifdef USING_VULKAN
		config.antialiasing_method = AntialiasingMethod::NONE;
#endif

		if (config.antialiasing_method == AntialiasingMethod::MSAA) {
			msg_error("yes msaa");

			msg_write("ms tex:");
			auto tex_ms = new TextureMultiSample(engine.width, engine.height, 4, "rgba:f16");
			msg_write("ms depth:");
			auto depth_ms = new TextureMultiSample(engine.width, engine.height, 4, "d:f32");
			msg_write("ms renderer:");
			//auto depth_ms = new nix::RenderBuffer(engine.width, engine.height, 4, "ds:u24i88");
			texture_renderer = new TextureRenderer("world-tex", {tex_ms, depth_ms}, {"samples=4"});

			multisample_resolver = new MultisampleResolver(tex_ms, depth_ms, hdr_tex, hdr_depth);
		} else {
			msg_error("no msaa");
			texture_renderer = new TextureRenderer("world-tex", {hdr_tex, hdr_depth});
		}

		texture_renderer->add_child(world_renderer);

		light_meter = new LightMeter(engine.resource_manager, hdr_tex);
	}
	void prepare(const RenderParams &params) override {
		prepare_basics();

		geo_renderer->prepare(params);

		render_cubemaps(params);
		prepare_lights(cam_main, main_rvd);

		if (scene_view.shadow_index >= 0) {
			shadow_renderer->set_scene(scene_view);
			shadow_renderer->render(params);
		}

		texture_renderer->prepare(params);

		auto scaled_params = params.with_area(dynamicly_scaled_area(texture_renderer->frame_buffer.get()));
		texture_renderer->render(scaled_params);

		if (multisample_resolver)
			multisample_resolver->render(scaled_params);

		if (hdr_resolver)
			hdr_resolver->prepare(params);


		if (light_meter and hdr_resolver) {
			light_meter->active = hdr_resolver->cam and hdr_resolver->cam->auto_exposure;
			if (light_meter->active) {
				light_meter->read();
				light_meter->setup();
				light_meter->adjust_camera(hdr_resolver->cam);
			}
		}
	}
	void draw(const RenderParams &params) override {
		hdr_resolver->draw(params);
	}
	void render_into_texture(FrameBuffer *fb, Camera *cam, RenderViewData &rvd) override {

	}
};


RenderPath* create_render_path(Camera *cam) {
	string type = config.get_str("renderer.path", "forward");

	if (type == "direct")
		return new RenderPathDirect(cam);
	if (type == "deferred")
		return new RenderPathComplex(cam, RenderPathType::Deferred);
	if (type == "pathtracing" or type == "raytracing")
		return new RenderPathComplex(cam, RenderPathType::PathTracing);
	return new RenderPathComplex(cam, RenderPathType::Forward);
}

