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

Picture::Picture(const vector &p, float w, float h, nix::Texture *tex, nix::Shader *_shader) {
	pos = p;
	width = w;
	height = h;
	texture = tex;
	col = White;
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

Picture::Picture(const vector &p, float w, float h, nix::Texture *tex) : Picture(p, w, h, tex, nullptr) {
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

void Picture::rebuild() {
	//dset->set({ubo}, {texture});
}

void vb_create_rect(nix::VertexBuffer *vb, const rect &s) {
	Array<vector> p = {vector(s.x1,s.y1,0), vector(s.x1,s.y2,0), vector(s.x2,s.y2,0),  vector(s.x1,s.y1,0), vector(s.x2,s.y2,0), vector(s.x2,s.y1,0)};
	Array<float> uv = {0,0, 0,1, 1,1,  0,0, 1,1, 1,0};
	vb->update(0, p);
	vb->update(1, p);
	vb->update(2, uv);
}

namespace gui {

nix::Shader *shader = nullptr;
nix::VertexBuffer *vertex_buffer = nullptr;
Array<Picture*> pictures;


void init() {
	vertex_buffer = new nix::VertexBuffer("3f,3f,2f");
	vb_create_rect(vertex_buffer, rect::ID);

	shader = nix::Shader::load("forward/2d.shader");
}

/*vulkan::RenderPass *rp) {
	Picture::render_pass = rp;
	Picture::shader = vulkan::Shader::load("2d.shader");
	Picture::pipeline = new vulkan::Pipeline(Picture::shader, rp, 0, 1);
	Picture::pipeline->set_blend(VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
	Picture::pipeline->set_z(false, false);
	Picture::pipeline->rebuild();

	Picture::vertex_buffer = new vulkan::VertexBuffer();
	Array<vulkan::Vertex1> vertices;
	vertices.add({vector(0,0,0), vector::EZ, 0,0});
	vertices.add({vector(0,1,0), vector::EZ, 0,1});
	vertices.add({vector(1,0,0), vector::EZ, 1,0});
	vertices.add({vector(1,1,0), vector::EZ, 1,1});
	Picture::vertex_buffer->build1i(vertices, {0,2,1, 1,2,3});
}*/

void reset() {
	for (auto *p: pictures)
		delete p;
	pictures.clear();
}

void add(Picture *p) {
	pictures.add(p);
}

void update() {
	for (auto *p: pictures) {
		p->rebuild();
	}
}

#if 0
void render(vulkan::CommandBuffer *cb, const rect &viewport) {
	cb->set_pipeline(Picture::pipeline);
	cb->set_viewport(viewport);

	for (auto *p: pictures) {
		if (p->user_shader)
			continue;

		UBOMatrices u;
		u.proj = matrix::translation(vector(-1,-1,0)) * matrix::scale(2,2,1);
		u.view = matrix::ID;
		u.model = matrix::translation(p->pos) * matrix::scale(p->width, p->height, 1);
		p->ubo->update(&u);

		cb->bind_descriptor_set(0, p->dset);
		cb->draw(Picture::vertex_buffer);
	}

	for (auto *p: pictures) {
		if (!p->user_shader)
			continue;
		cb->set_pipeline(p->user_pipeline);
		cb->set_viewport(viewport);

		UBOMatrices u;
		u.proj = matrix::translation(vector(-1,-1,0)) * matrix::scale(2,2,1);
		u.view = matrix::ID;
		u.model = matrix::translation(p->pos) * matrix::scale(p->width, p->height, 1);
		p->ubo->update(&u);

		cb->bind_descriptor_set(0, p->dset);
		cb->draw(Picture::vertex_buffer);
	}
}
#endif

}
