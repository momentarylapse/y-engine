/*
 * Picture.cpp
 *
 *  Created on: 04.01.2020
 *      Author: michi
 */

#include "Picture.h"
#include "../lib/nix/nix.h"
#include <iostream>


/*vulkan::Shader *Picture::shader = nullptr;
vulkan::Pipeline *Picture::pipeline = nullptr;
vulkan::VertexBuffer *Picture::vertex_buffer = nullptr;
vulkan::RenderPass *Picture::render_pass = nullptr;*/



struct UBOMatrices {
	alignas(16) matrix model;
	alignas(16) matrix view;
	alignas(16) matrix proj;
};


namespace gui {

Picture::Picture(const rect &r, nix::Texture *tex, nix::Shader *_shader) :
	Node(r)
{
	type = Type::PICTURE;
	source = rect::ID;
	texture = tex;
	bg_blur = 0;
#if 0
	ubo = new vulkan::UniformBuffer(sizeof(UBOMatrices));
	user_shader = _shader;
	user_pipeline = nullptr;

	if (user_shader) {
		user_pipeline = new vulkan::Pipeline(user_shader, render_pass, 0, 1);
	}
	dset = new vulkan::DescriptorSet({ubo}, {texture});
#endif
}

Picture::Picture(const rect &r, nix::Texture *tex) : Picture(r, tex, nullptr) {
}

Picture::~Picture() {
	/*
	delete ubo;
	delete dset;
//	if (user_shader)
//		delete user_shader;
	if (user_pipeline)
		delete user_pipeline;*/
}

//void Picture::rebuild() {
	//dset->set({ubo}, {texture});
//}

void Picture::__init__(const rect &r, nix::Texture *tex) {
	new(this) Picture(r, tex);
}

void Picture::__delete__() {
	this->Picture::~Picture();
}


}
