#pragma once

#include "../Renderer.h"

#ifdef USING_VULKAN

class TextureRenderer : public RenderTask {
public:
	owned<RenderPass> render_pass;
	shared<FrameBuffer> frame_buffer;
	shared_array<Texture> textures;
	bool clear_z = true;

	explicit TextureRenderer(const string& name, const shared_array<Texture>& tex, const Array<string>& options = {});
	~TextureRenderer() override;

	void set_area(const rect& area);
	bool override_area = false;
	rect user_area;

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
