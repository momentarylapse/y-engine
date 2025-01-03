/*
 * RendererFactory.h
 *
 *  Created on: 11 Oct 2023
 *      Author: michi
 */

#ifndef SRC_RENDERER_HELPER_RENDERERFACTORY_H_
#define SRC_RENDERER_HELPER_RENDERERFACTORY_H_

#include <graphics-fwd.h>

#include <renderer/Renderer.h>

struct GLFWwindow;
class Renderer;
class Camera;
class HDRRenderer;
class PostProcessor;
class WorldRenderer;
class TextureRenderer;
class MultisampleResolver;
class LightMeter;

class RenderPath : public Renderer {
public:
	RenderPath();
	~RenderPath() override;

	HDRRenderer* hdr_renderer = nullptr;
	PostProcessor* post_processor = nullptr;
	WorldRenderer* world_renderer = nullptr;
	TextureRenderer* texture_renderer = nullptr;
	MultisampleResolver* multisample_resolver = nullptr;
	LightMeter* light_meter = nullptr;
};

RenderPath* create_render_path(Camera *cam);
void create_full_renderer(GLFWwindow* window, Camera *cam);

#endif /* SRC_RENDERER_HELPER_RENDERERFACTORY_H_ */
