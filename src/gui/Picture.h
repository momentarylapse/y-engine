/*
 * Picture.h
 *
 *  Created on: 04.01.2020
 *      Author: michi
 */

#pragma once

#include "../graphics-fwd.h"
#include "Node.h"

namespace gui {

class Picture : public Node {
public:
	Picture(const rect &r, Texture *tex, const rect &source, Shader *shader);
	Picture(const rect &r, Texture *tex, const rect &source = rect::ID);
	virtual ~Picture();

	void __init2__(const rect &r, Texture *tex);
	void __init3__(const rect &r, Texture *tex, const rect &source);
	virtual void __delete__();

	rect source;

	float bg_blur;
	float angle;

	shared<Shader> shader;
	shared<Texture> texture;
	/*vulkan::Texture *texture;
	vulkan::UniformBuffer *ubo;
	vulkan::DescriptorSet *dset;
	vulkan::Shader *user_shader;
	vulkan::Pipeline *user_pipeline;*/

	//virtual void rebuild();

	/*static vulkan::Shader *shader;
	static vulkan::Pipeline *pipeline;
	static vulkan::VertexBuffer *vertex_buffer;
	static vulkan::RenderPass *render_pass;*/
};

}
