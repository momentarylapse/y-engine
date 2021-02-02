/*
 * Font.h
 *
 *  Created on: Feb 2, 2021
 *      Author: michi
 */

#pragma once

//#include "../lib/base/base.h"
//#include "gui.h"
#include "Node.h"

class Image;

namespace gui {

class Font {
public:
	void *face = nullptr;
	string name;

	static Font *load(const string &name);
	static void init_fonts();

	static void render_text(const string &str, Node::Align align, Image &im);

	static const int SOME_MARGIN;
};

}
