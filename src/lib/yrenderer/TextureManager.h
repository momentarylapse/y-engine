//
// Created by michi on 7/30/25.
//
#pragma once

#include <lib/ygraphics/graphics-fwd.h>
#include <lib/base/pointer.h>
#include <lib/base/map.h>
#include <lib/os/path.h>


class TextureManager {
public:
	explicit TextureManager(Context *ctx);
	Context* ctx;

	shared<Texture> load_texture(const Path& path);

	Path find_absolute_texture_path(const Path& path) const;

	Path texture_file(Texture* t) const;

	Path texture_dir;
	void clear();


	shared_array<Texture> textures;
	base::map<Path,Texture*> texture_map;

	shared<Texture> tex_white;
};

