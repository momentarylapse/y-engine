#include "ResourceManager.h"
#include <lib/yrenderer/ShaderManager.h>
#include <lib/os/filesystem.h>
#include <lib/os/file.h>
#include <lib/os/msg.h>
#include <lib/os/app.h>
#include <lib/image/image.h>
#include <y/EngineData.h>
#include <lib/ygraphics/graphics-impl.h>

#include <world/components/UserMesh.h>
#include <world/Material.h>
#include <world/ModelManager.h>

#ifdef USING_VULKAN
namespace vulkan {
	extern string overwrite_bindings;
	extern int overwrite_push_size;
}
#endif


ResourceManager::ResourceManager(::Context *_ctx) {
	ctx = _ctx;
	material_manager = new MaterialManager(this);
	model_manager = new ModelManager(this, material_manager);
	shader_manager = new ShaderManager(ctx);
	shader_manager->ignore_missing_files = engine.ignore_missing_files;


	if (ctx) {
#ifdef USING_VULKAN
		Image im;
		im.create(8, 8, White);
		tex_white = new Texture();
		tex_white->write(im);
#else
		tex_white = ctx->tex_white;
#endif
	}
}

xfer<Material> ResourceManager::load_material(const Path &filename) {
	return material_manager->load(filename);
}

xfer<Model> ResourceManager::load_model(const Path &filename) {
	return model_manager->load(filename);
}

Path guess_absolute_path(const Path &filename, const Array<Path> dirs) {
	if (filename.is_empty())
		return Path::EMPTY;

	if (filename.is_absolute())
		return filename;

	for (auto &d: dirs)
		if (os::fs::exists(d | filename))
			return d | filename;

	return Path::EMPTY;
	/*if (engine.ignore_missing_files) {
		msg_error("missing shader: " + filename.str());
		return Shader::load("");
	}
	throw Exception("missing shader: " + filename.str());
	return filename;*/
}




Path ResourceManager::texture_file(Texture* t) const {
	for (auto&& [key, _t]: texture_map)
		if (_t == t)
			return key;
	return "";
}


Path ResourceManager::find_absolute_texture_path(const Path& filename) const {
	return guess_absolute_path(filename, {texture_dir});
}

shared<Texture> ResourceManager::load_texture(const Path& filename) {
	if (filename.is_empty())
		return tex_white;

	Path fn = find_absolute_texture_path(filename);
	if (fn.is_empty()) {
		if (engine.ignore_missing_files) {
			msg_error("missing texture: " + str(filename));
			return tex_white;
		}
		throw Exception("missing texture: " + str(filename));
	}

	for (auto&& [key, t]: texture_map)
		if (fn == key) {
#ifdef USING_VULKAN
			return t;
#else
			return t->valid ? t : tex_white;
#endif
		}

	try {
#ifdef USING_VULKAN
		msg_write("loading texture: " + str(fn));
#endif
		auto t = Texture::load(fn);
		textures.add(t);
		texture_map.add({fn, t});
		return t;
	} catch(Exception &e) {
		if (!engine.ignore_missing_files)
			throw;
		msg_error(e.message());
		return tex_white;
	}
}

void ResourceManager::clear() {
	shader_manager->clear();
	textures.clear();
	texture_map.clear();
	material_manager->reset();
}


