/*
 * Text.cpp
 *
 *  Created on: 04.01.2020
 *      Author: michi
 */

#include "Text.h"

//#include "../lib/hui/hui.h"
#include "../lib/image/image.h"
#include <cairo/cairo.h>
#include <gtk/gtk.h>
#include <iostream>




void cairo_render_text(const string &font_name, float font_size, const string &text, Image &im) {
	bool failed = false;
	cairo_surface_t *surface;
	cairo_t *cr;

	// initial surface size guess
	int w_surf = 800;
	int h_surf = font_size * 2;

	surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w_surf, h_surf);
	cr = cairo_create(surface);

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

	if (w_used > w_surf)
		std::cerr << "Text: too large!\n";

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

	cairo_destroy(cr);
	cairo_surface_destroy(surface);
}

void render_text(const string &str, Image &im) {
	string font_name = "CAC Champagne";
	float font_size = 23;
	cairo_render_text(font_name, font_size, str, im);
}



Text::Text(const string &t, const vector &p, float h) : Picture(p, 1, h, new nix::Texture()) {
	text = t;
	rebuild();
}

Text::~Text() {
	delete texture;
}


void Text::rebuild() {
	Image im;
	render_text(text, im);
	texture->overwrite(im);
	//dset->set({ubo}, {texture});

	width = height * (float)im.width / (float)im.height / 1.33f;

}
