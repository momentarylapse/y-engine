#pragma once

#include <lib/ygraphics/graphics-fwd.h>
#include <lib/base/pointer.h>
#include <lib/base/map.h>
#include <lib/os/path.h>


struct string;
class MaterialManager;
class Material;
class ModelManager;
class Model;
class ShaderManager;

class ResourceManager {
public:
	explicit ResourceManager(Context *ctx);
	Context* ctx;
	MaterialManager* material_manager;
	ModelManager* model_manager;
	ShaderManager* shader_manager;

	shared<Texture> load_texture(const Path& path);
	xfer<Material> load_material(const Path &filename);
	xfer<Model> load_model(const Path &filename);

	Path find_absolute_texture_path(const Path& path) const;

	Path texture_file(Texture* t) const;

	Path texture_dir;
	void clear();


	shared_array<Texture> textures;
	base::map<Path,Texture*> texture_map;

	shared<Texture> tex_white;
};

