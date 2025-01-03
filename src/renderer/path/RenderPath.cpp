//
// Created by michi on 1/3/25.
//

#include "RenderPath.h"
#include "../base.h"
#include "../world/WorldRenderer.h"
#include "../post/ThroughShaderRenderer.h"
#include "../post/MultisampleResolver.h"
#include "../regions/RegionRenderer.h"
#include "../post/HDRRenderer.h"
#ifdef USING_VULKAN
	#include "../world/WorldRendererVulkan.h"
	#include "../world/WorldRendererVulkanForward.h"
	#include "../world/WorldRendererVulkanRayTracing.h"
#else
	#include "../world/WorldRendererGL.h"
	#include "../world/WorldRendererGLForward.h"
	#include "../world/WorldRendererGLDeferred.h"
#endif
#include "../helper/LightMeter.h"
#include <renderer/target/TextureRendererGL.h>
#include <renderer/target/TextureRendererVulkan.h>
#include <graphics-impl.h>
#include <world/Camera.h>
#include <y/EngineData.h>
#include <Config.h>
#include <lib/os/msg.h>


HDRRenderer *create_hdr_renderer(Camera *cam, Texture* tex, DepthBuffer* depth) {
	return new HDRRenderer(cam, tex, depth);
}

WorldRenderer *create_world_renderer(Camera *cam, const string& type) {
#ifdef USING_VULKAN
	if (type == "raytracing")
		return new WorldRendererVulkanRayTracing(device, cam, engine.width, engine.height);
	else
		return new WorldRendererVulkanForward(device, cam);
#else
	if (type == "deferred")
		return new WorldRendererGLDeferred(cam, engine.width, engine.height);
	else
		return new WorldRendererGLForward(cam);
#endif
}


RenderPath::RenderPath() : Renderer("path") {
}

RenderPath::~RenderPath() = default;

class RenderPathDirect : public RenderPath {
public:
	explicit RenderPathDirect(Camera* cam) {
		world_renderer = create_world_renderer(cam, "forward");
	}
	void prepare(const RenderParams &params) override {
		world_renderer->prepare(params);
	}
	void draw(const RenderParams &params) override {
		world_renderer->draw(params);
	}
};

class RenderPathHdr : public RenderPath {
public:
	explicit RenderPathHdr(Camera* cam, const string& type) {

		world_renderer = create_world_renderer(cam, type);

		auto hdr_tex = new Texture(engine.width, engine.height, "rgba:f16");
		hdr_tex->set_options("wrap=clamp,minfilter=nearest");
		hdr_tex->set_options("magfilter=" + config.resolution_scale_filter);
		auto hdr_depth = new DepthBuffer(engine.width, engine.height, "d:f32");

		hdr_renderer = create_hdr_renderer(cam, hdr_tex, hdr_depth);

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
			//auto depth_ms = new nix::RenderBuffer(engine.width, engine.height, 4, "d24s8");
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
		texture_renderer->prepare(params);

		auto scaled_params = params.with_area(dynamicly_scaled_area(texture_renderer->frame_buffer.get()));
		texture_renderer->render(scaled_params);

		if (multisample_resolver)
			multisample_resolver->render(scaled_params);

		hdr_renderer->prepare(params);


		if (light_meter) {
			light_meter->active = hdr_renderer->cam and hdr_renderer->cam->auto_exposure;
			if (light_meter->active) {
				light_meter->read();
				light_meter->setup();
				light_meter->adjust_camera(hdr_renderer->cam);
			}
		}
	}
	void draw(const RenderParams &params) override {
		hdr_renderer->draw(params);
	}
};


RenderPath* create_render_path(Camera *cam) {
	string type = config.get_str("renderer.path", "forward");

	if (type == "direct")
		return new RenderPathDirect(cam);
	return new RenderPathHdr(cam, type);
}

