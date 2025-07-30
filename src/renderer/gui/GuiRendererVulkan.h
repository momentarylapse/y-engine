/*
 * GuiRendererVulkan.h
 *
 *  Created on: 23 Nov 2021
 *      Author: michi
 */

#pragma once

#include <lib/yrenderer/Renderer.h>
#ifdef USING_VULKAN

class GuiRendererVulkan : public yrenderer::Renderer {
public:
	GuiRendererVulkan();

	void draw(const yrenderer::RenderParams& params) override;

	shared<Shader> shader;
	GraphicsPipeline* pipeline = nullptr;
	Array<DescriptorSet*> dset;
	Array<UniformBuffer*> ubo;
	owned<VertexBuffer> vb;
	void prepare_gui(FrameBuffer *source, const yrenderer::RenderParams& params);
	void draw_gui(CommandBuffer *cb, RenderPass *render_pass);
};

#endif
