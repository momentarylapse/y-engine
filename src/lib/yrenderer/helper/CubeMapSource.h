#ifndef CUBEMAPSOURCE_H
#define CUBEMAPSOURCE_H

#include <y/Component.h>
#include <lib/ygraphics/graphics-fwd.h>
#include <lib/base/pointer.h>
#include <lib/yrenderer/Renderer.h>
#include <lib/yrenderer/scene/RenderViewData.h>

namespace yrenderer {

class TextureRenderer;
class MeshEmitter;
class SceneRenderer;

class CubeMapSource : public Component {
public:
	CubeMapSource();
	~CubeMapSource() override;

	float min_depth, max_depth;
	shared<ygfx::CubeMap> cube_map;
	shared<ygfx::DepthBuffer> depth_buffer;
	shared<ygfx::FrameBuffer> frame_buffer[6];
#ifdef USING_VULKAN
	owned<ygfx::RenderPass> render_pass;
#endif
	int resolution;
	int update_rate;
	int counter;

	static const kaba::Class *_class;
};



class CubeMapRenderer : public RenderTask {
public:
	explicit CubeMapRenderer(SceneView& scene_view, shared_array<MeshEmitter> emitters);

	CubeMapSource* source = nullptr;
	void set_source(CubeMapSource* s);
	owned<SceneRenderer> scene_renderers[6];
	owned<TextureRenderer> texture_renderers[6];

	void render(const RenderParams& params) override;
};

}

#endif //CUBEMAPSOURCE_H
