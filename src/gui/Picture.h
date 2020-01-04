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
	Picture(const vector &pos, float w, float h, vulkan::Texture *tex);
	virtual ~Picture();

	vector pos;
	color col;
	float height;
	float width;

	vulkan::Texture *texture;
	vulkan::UBOWrapper *ubo;
	vulkan::DescriptorSet *dset;

	virtual void rebuild();

	static vulkan::Shader *shader;
	static vulkan::Pipeline *pipeline;
	static vulkan::VertexBuffer *vertex_buffer;
};

namespace gui {
	void init(vulkan::RenderPass *rp);
	void render(vulkan::CommandBuffer *cb, const rect &viewport);
	void update();
	void add(Picture *p);
}

#endif /* SRC_GUI_PICTURE_H_ */
