//
// Created by michi on 12/8/24.
//
#include "ThroughShaderRenderer.h"
#include "../base.h"
#include "../../graphics-impl.h"
#include "../../helper/PerformanceMonitor.h"


void apply_shader_data(Shader *s, const Any &shader_data);

ThroughShaderRenderer::ThroughShaderRenderer(const shared_array<Texture>& _textures, shared<Shader> _shader) : Renderer("shader") {
	textures = _textures;
	shader = _shader;
	vb_2d = new nix::VertexBuffer("3f,3f,2f");
	vb_2d->create_quad(rect::ID_SYM);
}

void ThroughShaderRenderer::set_source(const rect& area) {
	vb_2d->create_quad(rect::ID_SYM, area);
}

void ThroughShaderRenderer::draw(const RenderParams &params) {
	bool flip_y = params.target_is_window;

	PerformanceMonitor::begin(ch_draw);
	gpu_timestamp_begin(ch_draw);

	nix::bind_textures(weak(textures));
	nix::set_shader(shader.get());
	apply_shader_data(shader.get(), data);
	nix::set_projection_matrix(flip_y ? mat4::scale(1,-1,1) : mat4::ID);
	nix::set_view_matrix(mat4::ID);
	nix::set_model_matrix(mat4::ID);
	nix::set_cull(nix::CullMode::NONE);

	nix::set_z(false, false);

	nix::draw_triangles(vb_2d.get());

	nix::set_cull(nix::CullMode::BACK);
	gpu_timestamp_end(ch_draw);
	PerformanceMonitor::end(ch_draw);
}

