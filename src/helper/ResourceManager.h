#pragma once

#include <lib/ygraphics/graphics-fwd.h>
#include <lib/base/pointer.h>
#include <lib/os/path.h>


struct string;
class ModelManager;
class Model;

namespace yrenderer {
	class Material;
	class MaterialManager;
	class ShaderManager;
	class TextureManager;
}

class ResourceManager {
public:
	explicit ResourceManager(Context *ctx, const Path &texture_dir, const Path &material_dir, const Path &shader_dir);
	Context* ctx;
	yrenderer::MaterialManager* material_manager;
	ModelManager* model_manager;
	yrenderer::ShaderManager* shader_manager;
	yrenderer::TextureManager* texture_manager;

	shared<Texture> load_texture(const Path& path);
	xfer<yrenderer::Material> load_material(const Path &filename);
	xfer<Model> load_model(const Path &filename);

	void clear();
};

