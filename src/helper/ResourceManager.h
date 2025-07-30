#pragma once

#include <lib/ygraphics/graphics-fwd.h>
#include <lib/base/pointer.h>
#include <lib/os/path.h>


struct string;
class MaterialManager;
class Material;
class ModelManager;
class Model;
class ShaderManager;
class TextureManager;

class ResourceManager {
public:
	explicit ResourceManager(Context *ctx, const Path &texture_dir, const Path &shader_dir);
	Context* ctx;
	MaterialManager* material_manager;
	ModelManager* model_manager;
	ShaderManager* shader_manager;
	TextureManager* texture_manager;

	shared<Texture> load_texture(const Path& path);
	xfer<Material> load_material(const Path &filename);
	xfer<Model> load_model(const Path &filename);

	void clear();
};

