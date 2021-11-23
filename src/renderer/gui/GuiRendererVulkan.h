/*
 * GuiRendererVulkan.h
 *
 *  Created on: 23 Nov 2021
 *      Author: michi
 */

#pragma once

#include "../Renderer.h"
#ifdef USING_VULKAN

class GuiRendererVulkan : public Renderer {
public:
	GuiRendererVulkan(Renderer *parent);
	virtual ~GuiRendererVulkan();

	void draw() override;

	int ch_gui = -1;

	Shader *shader;
	Pipeline* pipeline;
	Array<DescriptorSet*> dset;
	Array<UniformBuffer*> ubo;
	VertexBuffer* vb;
	void prepare_gui(FrameBuffer *source);
	void draw_gui(CommandBuffer *cb);
};

#endif
