/*
 * Picture.cpp
 *
 *  Created on: 04.01.2020
 *      Author: michi
 */

#include "Picture.h"
#include "../lib/math/rect.h"
#include "../lib/math/vector.h"
#include "../lib/math/matrix.h"
#include "../graphics-impl.h"
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

Picture::Picture(const rect &r, Texture *tex, const rect &s, Shader *_shader) :
	Node(r)
{
	type = Type::PICTURE;
	source = s;
	texture = tex;
	bg_blur = 0;
	angle = 0;
	visible = true;
	shader = nullptr;
}

Picture::Picture(const rect &r, Texture *tex, const rect &s) : Picture(r, tex, s, nullptr) {
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

void Picture::__init__(const rect &r, Texture *tex, const rect &s) {
	new(this) Picture(r, tex, s);
	source = s;
}

void Picture::__delete__() {
	this->Picture::~Picture();
}


}
