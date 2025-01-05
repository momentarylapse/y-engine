//
// Created by michi on 1/5/25.
//

#ifndef BINDABLE_H
#define BINDABLE_H

#include <lib/base/base.h>
#include <lib/base/pointer.h>
#include <lib/any/any.h>
#include "../../graphics-fwd.h"

struct RenderParams;

struct Binding {
	enum class Type {
		Texture,
		Image,
		UniformBuffer,
		StorageBuffer
	};
	int index;
	Type type;
	void* p;
};

class Bindable {
public:
	explicit Bindable(Shader* shader);
	Array<Binding> bindings;
	Any shader_data;

	void bind_texture(int index, Texture* texture);
	void bind_textures(int index0, const Array<Texture*>& textures);
	void bind_image(int index, ImageTexture* image);
	void bind_uniform_buffer(int index, Buffer* buffer);
	void bind_storage_buffer(int index, Buffer* buffer);

	void apply_bindings(Shader* shader, const RenderParams& params);

#ifdef USING_VULKAN
	owned<vulkan::DescriptorPool> pool;
	owned<vulkan::DescriptorSet> dset;
#endif
};

class mat4;
class vec2;
class vec3;

Any mat4_to_any(const mat4& m);
Any vec2_to_any(const vec2& v);
Any vec3_to_any(const vec3& v);


#endif //BINDABLE_H
