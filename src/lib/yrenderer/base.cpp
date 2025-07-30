/*
 * base.cpp
 *
 *  Created on: 21 Nov 2021
 *      Author: michi
 */

#include "base.h"
#include <lib/ygraphics/graphics-impl.h>
#include <lib/image/image.h>

namespace yrenderer {

void Context::_create_default_textures() {
	tex_white = new ygfx::Texture();
	tex_black = new ygfx::Texture();
	tex_white->write(Image(16, 16, White));
	tex_black->write(Image(16, 16, Black));
}

}



