//
// Created by michi on 12/8/24.
//
#include "ThroughShaderRenderer.h"
#include "../base.h"
#include "../../graphics-impl.h"
#include "../../helper/PerformanceMonitor.h"
#include <lib/base/iter.h>
#include <lib/math/mat4.h>
#include <lib/math/vec2.h>
#include <lib/os/msg.h>

#ifdef USING_VULKAN
void apply_shader_data(CommandBuffer* cb, const Any &shader_data) {
	if (shader_data.is_empty()) {
		return;
	} else if (shader_data.is_dict()) {
		char temp[256];
		int max_used = 0;
		for (auto &key: shader_data.keys()) {
			auto &val = shader_data[key];
			auto x = key.explode(":");
			if (x.num < 2) {
				msg_write("invalid shader data key (:offset missing): " + key);
				continue;
			}
			string name = x[0];
			int offset = x[1]._int();
			if (val.is_float()) {
				*(float*)&temp[offset] = val.as_float();
				max_used = max(max_used, offset + 4);
			} else if (val.is_int()) {
				*(int*)&temp[offset] = val.as_int();
				max_used = max(max_used, offset + 4);
			} else if (val.is_bool()) {
				*(bool*)&temp[offset] = val.as_bool();
				max_used = max(max_used, offset + 1);
			} else if (val.is_list()) {
				for (int i=0; i<val.as_list().num; i++)
					*(float*)&temp[offset + i * 4] = val.as_list()[i].as_float();
				max_used = max(max_used, offset + 4 * val.as_list().num);
			} else {
				msg_write("invalid shader data item: " + val.str());
			}
		}
	//	msg_write(bytes(&temp, max_used).hex());
		cb->push_constant(0, max_used, &temp);
	} else {
		msg_write("invalid shader data: " + shader_data.str());
	}
}
#else
void apply_shader_data(Shader *s, const Any &shader_data) {
	if (shader_data.is_empty()) {
		return;
	} else if (shader_data.is_dict()) {
		for (auto &key: shader_data.keys()) {
			auto &val = shader_data[key];
			string name = key.explode(":")[0];
			if (val.is_float()) {
				s->set_float(name, val.as_float());
			} else if (val.is_int()) {
				s->set_int(name, val.as_int());
			} else if (val.is_bool()) {
				s->set_int(name, (int)val.as_bool());
			} else if (val.is_list()) {
				float ff[4];
				for (int i=0; i<val.as_list().num; i++)
					ff[i] = val.as_list()[i].as_float();
				s->set_floats(name, ff, val.as_list().num);
			} else {
				msg_write("invalid shader data item: " + val.str());
			}
		}
	} else {
		msg_write("invalid shader data: " + shader_data.str());
	}
}
#endif

Any mat4_to_any(const mat4& m) {
	Any a = Any::EmptyList;
	for (int i=0; i<16; i++)
		a.list_set(i, ((float*)&m)[i]);
	return a;
}
Any vec2_to_any(const vec2& v) {
	Any a = Any::EmptyList;
	a.list_set(0, v.x);
	a.list_set(1, v.y);
	return a;
}

ThroughShaderRenderer::ThroughShaderRenderer(const string& name, const shared_array<Texture>& _textures, shared<Shader> _shader) : Renderer(name) {
	textures = _textures;
	shader = _shader;
	vb_2d = new VertexBuffer("3f,3f,2f");
	vb_2d->create_quad(rect::ID_SYM);
	current_area = rect::ID;

#ifdef USING_VULKAN
	dset = pool->create_set("buffer,sampler,sampler,sampler,sampler,sampler");

	for (auto&& [i, t]: enumerate(weak(textures)))
		dset->set_texture(1 + i, t);
	dset->update();
#endif
}

void ThroughShaderRenderer::set_source(const rect& area) {
	if (area == current_area)
		return;
	current_area = area;;
	vb_2d->create_quad(rect::ID_SYM, area);
}

void ThroughShaderRenderer::draw(const RenderParams &params) {
#ifdef USING_VULKAN
	auto cb = params.command_buffer;

	PerformanceMonitor::begin(channel);
	gpu_timestamp_begin(params, channel);

	if (!pipeline) {
		pipeline = new vulkan::GraphicsPipeline(shader.get(), params.render_pass, 0, "triangles", "3f,3f,2f");
		pipeline->set_culling(CullMode::NONE);
		pipeline->set_z(false, false);
		pipeline->rebuild();
	}

	cb->bind_pipeline(pipeline);
	cb->bind_descriptor_set(0, dset);
	apply_shader_data(cb, data);
	cb->draw(vb_2d.get());


	gpu_timestamp_end(params, channel);
	PerformanceMonitor::end(channel);
#else
	bool flip_y = params.target_is_window;

	PerformanceMonitor::begin(channel);
	gpu_timestamp_begin(params, channel);

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

	gpu_timestamp_end(params, channel);
	PerformanceMonitor::end(channel);
#endif
}

