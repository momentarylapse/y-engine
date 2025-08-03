/*
 * base.cpp
 *
 *  Created on: 21 Nov 2021
 *      Author: michi
 */

#include "Context.h"
#include "TextureManager.h"
#include "ShaderManager.h"
#include "Material.h"
#include <lib/ygraphics/graphics-impl.h>
#include <lib/image/image.h>

namespace yrenderer {

void Context::_create_default_textures() {
	tex_white = new ygfx::Texture();
	tex_black = new ygfx::Texture();
	tex_white->write(Image(16, 16, White));
	tex_black->write(Image(16, 16, Black));
}

void Context::create_managers(const Path &texture_dir, const Path &shader_dir, const Path &material_dir) {
	texture_manager = new TextureManager(context, texture_dir);
	shader_manager = new ShaderManager(context, shader_dir);
	material_manager = new MaterialManager(this, material_dir);
}

xfer<Material> Context::load_material(const Path &filename) const {
	return material_manager->load(filename);
}

shared<ygfx::Texture> Context::load_texture(const Path& filename) const {
	return texture_manager->load_texture(filename);
}


}



