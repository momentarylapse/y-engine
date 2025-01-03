/*
 * RendererFactory.cpp
 *
 *  Created on: 11 Oct 2023
 *      Author: michi
 */

#include "RendererFactory.h"
#include "../base.h"
#include <graphics-impl.h>
#include "../world/WorldRenderer.h"
#include "../post/ThroughShaderRenderer.h"
#include "../post/MultisampleResolver.h"
#include "../regions/RegionRenderer.h"
#include "../post/HDRRenderer.h"
#ifdef USING_VULKAN
	#include "../world/WorldRendererVulkan.h"
	#include "../world/WorldRendererVulkanForward.h"
	#include "../world/WorldRendererVulkanRayTracing.h"
	#include "../gui/GuiRendererVulkan.h"
	#include "../post/PostProcessorVulkan.h"
	#include "../target/WindowRendererVulkan.h"
#else
	#include "../world/WorldRendererGL.h"
	#include "../world/WorldRendererGLForward.h"
	#include "../world/WorldRendererGLDeferred.h"
	#include "../gui/GuiRendererGL.h"
	#include "../post/PostProcessorGL.h"
	#include "../target/WindowRendererGL.h"
#endif
#include <y/EngineData.h>
#include <world/Camera.h>
#include <lib/os/msg.h>
#if __has_include(<lib/hui_minimal/hui.h>)
#include <lib/hui_minimal/hui.h>
#elif __has_include(<lib/hui/hui.h>)
#include <lib/hui/hui.h>
#endif
#include <helper/PerformanceMonitor.h>
#include <Config.h>
#include <helper/ResourceManager.h>
#include <helper/Scheduler.h>

#include <lib/image/image.h>
#include <renderer/target/TextureRendererGL.h>
#include <renderer/target/TextureRendererVulkan.h>

#include "LightMeter.h"


string render_graph_str(Renderer *r) {
	string s = PerformanceMonitor::get_name(r->channel);
	if (r->children.num == 1)
		s += " <<< " + render_graph_str(r->children[0]);
	if (r->children.num >= 2) {
		Array<string> ss;
		for (auto c: r->children)
			ss.add(render_graph_str(c));
		s += " <<< (" + implode(ss, ", ") + ")";
	}
	return s;
}

void print_render_chain() {
	msg_write("------------------------------------------");
	msg_write("CHAIN:  " + render_graph_str(engine.window_renderer));
	msg_write("------------------------------------------");
}


WindowRenderer *create_window_renderer(GLFWwindow* window) {
#ifdef HAS_LIB_GLFW
#ifdef USING_VULKAN
	return WindowRendererVulkan::create(window, device);
#else
	return new WindowRendererGL(window);
#endif
#else
	return nullptr;
#endif
}

Renderer *create_gui_renderer() {
#ifdef USING_VULKAN
	return new GuiRendererVulkan();
#else
	return new GuiRendererGL();
#endif
}

RegionRenderer *create_region_renderer() {
	return new RegionRenderer();
}

HDRRenderer *create_hdr_renderer(Camera *cam, Texture* tex, DepthBuffer* depth) {
	return new HDRRenderer(cam, tex, depth);
}

PostProcessor *create_post_processor() {
#ifdef USING_VULKAN
	return new PostProcessorVulkan();
#else
	return new PostProcessorGL(engine.width, engine.height);
#endif
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

/*class TextureWriter : public Renderer {
public:
	shared<Texture> texture;
	TextureWriter(shared<Texture> t) : Renderer("www") {
		texture = t;
	}
	void prepare(const RenderParams& params) override {
		Renderer::prepare(params);

		Image i;
		texture->read(i);
		i.save("o.bmp");
	}
};*/

void create_full_renderer(GLFWwindow* window, Camera *cam) {
	try {
		engine.window_renderer = create_window_renderer(window);
		engine.region_renderer = create_region_renderer();
		auto rp = create_render_path(cam);
		engine.render_paths.add(rp);
		engine.gui_renderer = create_gui_renderer();
		engine.window_renderer->add_child(engine.region_renderer);
		engine.region_renderer->add_region(rp, rect::ID, 0);
		engine.region_renderer->add_region(engine.gui_renderer, rect::ID, 999);

		if (false) {
			int N = 256;
			Image im;
			im.create(N, N, Black);
			for (int i = 0; i < N; i++)
				for (int j = 0; j < N; j++)
					im.set_pixel(i, j, ((i/16+j/16)%2 == 0) ? Black : White);
			shared tex = new Texture();
			tex->write(im);
			auto shader = engine.resource_manager->load_shader("forward/blur.shader");
			auto tsr = new ThroughShaderRenderer("blur", {tex}, shader);
			Any axis_x, axis_y;
			axis_x.list_set(0, 1.0f);
			axis_x.list_set(1, 0.0f);
			axis_y.list_set(0, 0.0f);
			axis_y.list_set(1, 1.0f);
			Any data;
			data.dict_set("radius:8", 5.0f);
			data.dict_set("threshold:12", 0.0f);
			data.dict_set("axis:0", axis_x);
			tsr->data = data;
			// tsr:  tex -> shader -> ...

			shared tex2 = new Texture(N, N, "rgba:i8");
#ifdef USING_VULKAN
			shared<Texture> depth2 = new DepthBuffer(N, N, "d:f32", true);
#else
			shared<Texture> depth2 = new DepthBuffer(N, N, "d24s8");
#endif
			auto tr = new TextureRenderer("tex", {tex2, depth2});
			tr->use_params_area = false;
			tr->add_child(tsr);
			// tr:  ... -> tex2

			auto tsr2 = new ThroughShaderRenderer("text", {tex2}, shader);
			data.dict_set("radius:8", 5.0f);
			data.dict_set("threshold:12", 0.0f);
			data.dict_set("axis:0", axis_y);
			tsr2->data = data;
			tsr2->add_child(tr);
			// tsr2:  tex2 -> shader -> ...

			engine.window_renderer->add_child(tsr2);
		}

	} catch(Exception &e) {
#if __has_include(<lib/hui_minimal/hui.h>)
		hui::ShowError(e.message());
#endif
		throw e;
	}
	print_render_chain();
}
