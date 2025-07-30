#include "ResourceManager.h"
#include <lib/yrenderer/ShaderManager.h>
#include <lib/yrenderer/TextureManager.h>
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


ResourceManager::ResourceManager(::Context *_ctx, const Path &texture_dir, const Path &shader_dir) {
	ctx = _ctx;
	material_manager = new MaterialManager(this);
	model_manager = new ModelManager(this, material_manager);
	shader_manager = new ShaderManager(ctx, shader_dir);
	shader_manager->ignore_missing_files = engine.ignore_missing_files;
	texture_manager = new TextureManager(ctx, texture_dir);
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



shared<Texture> ResourceManager::load_texture(const Path& filename) {
	return texture_manager->load_texture(filename);
}

void ResourceManager::clear() {
	shader_manager->clear();
	texture_manager->clear();
	material_manager->reset();
}


