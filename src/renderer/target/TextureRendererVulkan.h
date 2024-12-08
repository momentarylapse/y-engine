#pragma once

#include "../Renderer.h"

#ifdef USING_VULKAN

class TextureRenderer : public RenderTask {
public:
	RenderPass* render_pass;
	FrameBuffer* frame_buffer;
	shared_array<Texture> textures;
	bool use_params_area = true;

	explicit TextureRenderer(const shared_array<Texture>& tex);
	~TextureRenderer() override;

	// TODO move to explicit/dependency graph
	void prepare(const RenderParams& params) override;

	void render(const RenderParams& params) override;
};


// TODO "task executor"...
class HeadlessRenderer : public RenderTask {
public:
	vulkan::Device* device;
	CommandBuffer* command_buffer;
	vulkan::Fence* fence;

	owned<TextureRenderer> texture_renderer;

	HeadlessRenderer(vulkan::Device* d, const shared_array<Texture>& tex);
	~HeadlessRenderer() override;

	// TODO move to explicit/dependency graph
	void prepare(const RenderParams& params) override;

	void render(const RenderParams& params) override;

	RenderParams create_params(const rect& area) const;
};

#endif
