/*
 * Text.cpp
 *
 *  Created on: 04.01.2020
 *      Author: michi
 */

#include "Text.h"

//#include "../lib/hui/hui.h"
#include "../lib/image/image.h"
#include "../meta.h"
#include <cairo/cairo.h>
#include <iostream>




void cairo_render_text(const string &font_name, float font_size, const string &text, Image &im) {

	// initial surface size guess
	int w_surf = 200;
	int h_surf = font_size * 2;

	for (int i=0; i<2; i++) {

		cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w_surf, h_surf);
		cairo_t *cr = cairo_create(surface);

		cairo_set_source_rgba(cr, 0, 0, 0, 1);
		cairo_rectangle(cr, 0, 0, w_surf, h_surf);
		cairo_fill(cr);

		int x = 0, y = 0;

		cairo_set_source_rgba(cr, 1, 1, 1, 1);

		PangoLayout *layout = pango_cairo_create_layout(cr);
		PangoFontDescription *desc = pango_font_description_from_string((font_name + "," + f2s(font_size, 1)).c_str());
		pango_layout_set_font_description(layout, desc);
		pango_font_description_free(desc);

		pango_layout_set_text(layout, (char*)text.data, text.num);
		//int baseline = pango_layout_get_baseline(layout) / PANGO_SCALE;
		int w_used, h_used;
		pango_layout_get_pixel_size(layout, &w_used, &h_used);


		if ((w_used <= w_surf and h_used <= h_surf) or (i == 1)) {


			pango_cairo_show_layout(cr, layout);
			g_object_unref(layout);

			cairo_surface_flush(surface);
			unsigned char *c0 = cairo_image_surface_get_data(surface);
			im.create(w_used, h_used, White);
			for (int y=0;y<h_used;y++) {
				unsigned char *c = c0 + 4 * y * w_surf;
				for (int x=0;x<w_used;x++) {
					float a = (float)c[1] / 255.0f;
					im.set_pixel(x, y, color(a, 1, 1, 1));
					c += 4;
				}
			}
			im.alpha_used = true;

			// finished
			i = 666;
		}

		w_surf = w_used;
		h_surf = h_used;
		//if (w_used > w_surf)
		//	std::cerr << "Text: too large!\n";
		cairo_destroy(cr);
		cairo_surface_destroy(surface);
	}
}

void render_text(const string &str, Image &im) {
	string font_name = "CAC Champagne";
	float font_size = 32;
	cairo_render_text(font_name, font_size, str, im);
}

namespace gui {


Text::Text(const string &t, float h, float x, float y) : Picture(rect::ID, nullptr) {
	type = Type::TEXT;
	margin = rect(x, h/6, y, h/6);
	font_size = h;
	if (t != ":::fake:::")
		set_text(t);
}

Text::~Text() {
	if (texture)
		delete texture;
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
	render_text(text, im);
	texture = new nix::Texture();

	texture->overwrite(im);
	//dset->set({ubo}, {texture});

	float h = font_size * text.explode("\n").num;
	float w = h * (float)im.width / (float)im.height;
	if (align & Align::NONSQUARE)
		 w /= engine.physical_aspect_ratio;
	area.x2 = area.x1 + w;
	area.y2 = area.y1 + h;
}

void Text::set_text(const string &t) {
	text = t;
	rebuild();
}

}
