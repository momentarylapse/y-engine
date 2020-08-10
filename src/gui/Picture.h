/*
 * Picture.h
 *
 *  Created on: 04.01.2020
 *      Author: michi
 */

#ifndef SRC_GUI_PICTURE_H_
#define SRC_GUI_PICTURE_H_

#include "Node.h"

namespace gui {

class Picture : public Node {
public:
	Picture(const rect &r, nix::Texture *tex, nix::Shader *shader);
	Picture(const rect &r, nix::Texture *tex);
	virtual ~Picture();

	void __init__(const rect &r, nix::Texture *tex);
	virtual void __delete__();

	rect source;

	float bg_blur;

	nix::Texture *texture;
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

#endif /* SRC_GUI_PICTURE_H_ */
