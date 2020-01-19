/*
 * Picture.h
 *
 *  Created on: 04.01.2020
 *      Author: michi
 */

#ifndef SRC_GUI_PICTURE_H_
#define SRC_GUI_PICTURE_H_

#include "../lib/vulkan/vulkan.h"

class Picture {
public:
	Picture(const vector &pos, float w, float h, const Array<vulkan::Texture*> &tex, vulkan::Shader *shader);
	Picture(const vector &pos, float w, float h, vulkan::Texture *tex);
	virtual ~Picture();

	vector pos;
	color col;
	float height;
	float width;

	Array<vulkan::Texture*> textures;
	vulkan::UniformBuffer *ubo;
	vulkan::DescriptorSet *dset;
	vulkan::Shader *user_shader;
	vulkan::Pipeline *user_pipeline;

	virtual void rebuild();

	static vulkan::Shader *shader;
	static vulkan::Pipeline *pipeline;
	static vulkan::VertexBuffer *vertex_buffer;
	static vulkan::RenderPass *render_pass;
};

namespace gui {
	void init(vulkan::RenderPass *rp);
	void reset();
	void render(vulkan::CommandBuffer *cb, const rect &viewport);
	void update();
	void add(Picture *p);
}

#endif /* SRC_GUI_PICTURE_H_ */
