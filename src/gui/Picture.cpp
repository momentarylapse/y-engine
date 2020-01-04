/*
 * Picture.cpp
 *
 *  Created on: 04.01.2020
 *      Author: michi
 */

#include "Picture.h"
#include <iostream>

vulkan::Shader *Picture::shader = nullptr;
vulkan::Pipeline *Picture::pipeline = nullptr;
vulkan::VertexBuffer *Picture::vertex_buffer = nullptr;

static Array<Picture*> pictures;


struct UBOMatrices {
	alignas(16) matrix model;
	alignas(16) matrix view;
	alignas(16) matrix proj;
};

Picture::Picture(const vector &p, float w, float h, vulkan::Texture *tex) {
	pos = p;
	width = w;
	height = h;
	texture = tex;
	col = White;
	ubo = new vulkan::UBOWrapper(sizeof(UBOMatrices));
	dset = new vulkan::DescriptorSet(shader->descr_layouts[0], {ubo}, {texture});
}

Picture::~Picture() {
	delete ubo;
	delete dset;
}

void Picture::rebuild() {
	dset->set({ubo}, {texture});
}

namespace gui {

void init(vulkan::RenderPass *rp) {
	Picture::shader = vulkan::Shader::load("2d.shader");
	Picture::pipeline = vulkan::Pipeline::build(Picture::shader, rp, 1, false);
	Picture::pipeline->set_dynamic({"viewport"});
	Picture::pipeline->set_blend(VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
	Picture::pipeline->create();

	Picture::vertex_buffer = new vulkan::VertexBuffer();
	Array<vulkan::Vertex1> vertices;
	vertices.add({vector(0,0,0), vector::EZ, 0,0});
	vertices.add({vector(0,1,0), vector::EZ, 0,1});
	vertices.add({vector(1,0,0), vector::EZ, 1,0});
	vertices.add({vector(1,1,0), vector::EZ, 1,1});
	Picture::vertex_buffer->build1i(vertices, {0,2,1, 1,2,3});
}

void add(Picture *p) {
	pictures.add(p);
}

void update() {
	for (auto *p: pictures) {
		p->rebuild();
	}
}

void render(vulkan::CommandBuffer *cb, const rect &viewport) {
	cb->set_pipeline(Picture::pipeline);
	cb->set_viewport(viewport);

	for (auto *p: pictures) {

		UBOMatrices u;
		u.proj = (matrix::translation(vector(-1,-1,0)) * matrix::scale(2,2,1)).transpose();
		u.view = matrix::ID.transpose();
		u.model = (matrix::translation(p->pos) * matrix::scale(p->width, p->height, 1)).transpose();
		p->ubo->update(&u);

		cb->bind_descriptor_set(0, p->dset);
		cb->draw(Picture::vertex_buffer);
	}
}

}
