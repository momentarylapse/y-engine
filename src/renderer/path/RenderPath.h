//
// Created by michi on 1/3/25.
//

#ifndef RENDERPATH_H
#define RENDERPATH_H


#include <renderer/Renderer.h>

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

#endif //RENDERPATH_H
