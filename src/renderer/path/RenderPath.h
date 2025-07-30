//
// Created by michi on 1/3/25.
//

#ifndef RENDERPATH_H
#define RENDERPATH_H


#include <lib/yrenderer//Renderer.h>
#include <lib/yrenderer/scene/SceneView.h>

class Camera;
class PostProcessor;
class WorldRenderer;
class LightMeter;

namespace yrenderer {
	class CubeMapRenderer;
	class SceneRenderer;
	class HDRResolver;
	class TextureRenderer;
	class MultisampleResolver;
	class GeometryRenderer;
	class ShadowRenderer;
	struct RenderViewData;
	class CubeMapSource;
	enum class RenderPathType;
}

class RenderPath : public yrenderer::Renderer {
public:
	explicit RenderPath(yrenderer::RenderPathType type, Camera* cam);
	~RenderPath() override;

	yrenderer::RenderPathType type;

	float shadow_box_size;
	int shadow_resolution;
	Camera* cam;
	yrenderer::SceneView scene_view;

	WorldRenderer* world_renderer = nullptr;
	owned<yrenderer::ShadowRenderer> shadow_renderer;

	// post processing
	yrenderer::HDRResolver* hdr_resolver = nullptr;
	PostProcessor* post_processor = nullptr;
	yrenderer::TextureRenderer* texture_renderer = nullptr;
	yrenderer::MultisampleResolver* multisample_resolver = nullptr;
	LightMeter* light_meter = nullptr;

	Renderer* main_renderer = nullptr;


	void create_shadow_renderer();
	void create_cube_renderer();
	void create_post_processing(Renderer* source);

	void prepare(const yrenderer::RenderParams& params) override;
	void draw(const yrenderer::RenderParams& params) override;

	//virtual void render_into_texture(FrameBuffer *fb, Camera *cam, RenderViewData &rvd) {};
	void render_into_cubemap(yrenderer::CubeMapSource& source);

	void prepare_basics();
	void render_cubemaps(const yrenderer::RenderParams& params);
	void prepare_instanced_matrices();

	yrenderer::CubeMapSource* cube_map_source = nullptr;
	owned<yrenderer::CubeMapRenderer> cube_map_renderer;
	void suggest_cube_map_pos();
};

WorldRenderer *create_world_renderer(yrenderer::SceneView& scene_view, yrenderer::RenderPathType type);

#endif //RENDERPATH_H
