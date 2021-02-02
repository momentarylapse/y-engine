/*
 * Text.cpp
 *
 *  Created on: 04.01.2020
 *      Author: michi
 */

#include "Text.h"
#include "Font.h"
#include "../lib/image/image.h"

//#include "../lib/hui/hui.h"
#include "../y/EngineData.h"


namespace gui {


Text::Text(const string &t, float h, float x, float y) : Picture(rect::ID, nullptr) {
	type = Type::TEXT;
	margin = rect(x, h/6, y, h/10);
	font = Font::_default;
	font_size = h;
	if (t != ":::fake:::")
		set_text(t);
}

Text::~Text() {
}

void Text::__init2__(const string &t, float h) {
	new(this) Text(t, h, h/6, h/6);
}

void Text::__init4__(const string &t, float h, float x, float y) {
	new(this) Text(t, h, x, y);
}

void Text::__delete__() {
	this->Text::~Text();
}

void Text::rebuild() {
	Image im;
	font->render_text(text, align, im);
	if (texture == nullptr)
		texture = new nix::Texture();

	texture->overwrite(im);
	//dset->set({ubo}, {texture});

	float dx = (float)Font::SOME_MARGIN/(float)im.width;
	float dy = (float)Font::SOME_MARGIN/(float)im.height;
	source = rect(dx, 1.0f - dx, dy, 1.0f - dy);
	height = font_size * Font::LINE_FACTOR * text.explode("\n").num;
	width = height * (float)(im.width - Font::SOME_MARGIN*2) / (float)(im.height - Font::SOME_MARGIN*2);
	if (align & Align::NONSQUARE)
		 width /= engine.physical_aspect_ratio;
}

void Text::set_text(const string &t) {
	text = t;
	rebuild();
}

}
