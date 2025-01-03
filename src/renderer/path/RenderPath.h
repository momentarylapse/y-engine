//
// Created by michi on 1/3/25.
//

#ifndef RENDERPATH_H
#define RENDERPATH_H


#include <renderer/Renderer.h>
#include "../world/geometry/SceneView.h"
#include "../world/geometry/RenderViewData.h"

class Camera;
class HDRRenderer;
class PostProcessor;
class WorldRenderer;
class TextureRenderer;
class MultisampleResolver;
class LightMeter;
class GeometryRenderer;
class ShadowRenderer;
struct RenderViewData;
class CubeMapSource;

enum class RenderPathType {
	Direct,
	Forward,
	Deferred,
	PathTracing
};

class RenderPath : public Renderer {
public:
	explicit RenderPath(RenderPathType type, Camera* cam);
	~RenderPath() override;

	RenderPathType type = RenderPathType::Direct;

	float shadow_box_size;
	int shadow_resolution;
	SceneView scene_view;

	HDRRenderer* hdr_renderer = nullptr;
	PostProcessor* post_processor = nullptr;
	WorldRenderer* world_renderer = nullptr;
	TextureRenderer* texture_renderer = nullptr;
	MultisampleResolver* multisample_resolver = nullptr;
	LightMeter* light_meter = nullptr;

	owned<GeometryRenderer> geo_renderer;
	owned<ShadowRenderer> shadow_renderer;
	RenderViewData main_rvd;

	void create_shadow_renderer();
	void create_geometry_renderer();

	virtual void render_into_texture(FrameBuffer *fb, Camera *cam, RenderViewData &rvd) {};
	void render_into_cubemap(CubeMapSource& source);

	void prepare_basics();
	void prepare_lights(Camera *cam, RenderViewData &rvd);

	CubeMapSource* cube_map_source = nullptr;
	void suggest_cube_map_pos();
};

#endif //RENDERPATH_H
