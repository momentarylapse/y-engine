/*
 * Font.cpp
 *
 *  Created on: Feb 2, 2021
 *      Author: michi
 */

#include "Font.h"
#include "gui.h"
#include "../Config.h"
#include "../lib/image/image.h"

//#define USE_CAIRO 1
#define USE_FREETYPE 1


#ifdef USE_CAIRO
#include <cairo/cairo.h>
#endif

#ifdef USE_FREETYPE

#include <ft2build.h>
#include FT_FREETYPE_H

#endif

namespace gui {


#ifdef USE_FREETYPE

FT_Library ft2 = nullptr;
//FT_Face face = nullptr;

#endif

const float Font::FONT_SIZE = 40.0f;
//const float Font::LINE_GAP = 0;//.3f;
//const float Font::LINE_Y_OFFSET = 1;//0.84f;



Array<Font*> fonts;
Font *Font::_default = nullptr;


void *ft_load_font(const string &name, float font_size);

Font *Font::load(const string &name) {
	if (name == "" and Font::_default)
		return Font::_default;

	for (auto f: fonts)
		if (f->name == name)
			return f;

	msg_write("loading font " + name);
	auto f = new Font;
	f->name = name;
	f->face = ft_load_font(name, FONT_SIZE);
	f->line_height = (((FT_Face)f->face)->size->metrics.height >> 6);//FONT_SIZE * (1.0f + LINE_GAP);
	f->line_y_offset = (float)(((FT_Face)f->face)->size->metrics.ascender >> 6) / (float)(((FT_Face)f->face)->size->metrics.height >> 6);
	fonts.add(f);
	return f;
}


#ifdef USE_CAIRO

void cairo_render_text(const string &font_name, float font_size, const string &text, gui::Node::Align align, Image &im) {

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

		if (align & gui::Node::Align::RIGHT)
			pango_layout_set_alignment(layout, PANGO_ALIGN_RIGHT);
		else if (align & gui::Node::Align::CENTER_H)
			pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);
		else
			pango_layout_set_alignment(layout, PANGO_ALIGN_LEFT);
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

#else


Path find_system_font_file(const string &name) {
	Path base = "/usr/share/fonts";
	auto list = dir_search(base, "*.ttf", "fr");
	for (auto &f: list)
		if (f.basename_no_ext() == name or f.basename_no_ext() == name + "-Regular")
			return base << f;
	//return "/usr/share/fonts/TTF/DejaVuSansMono.ttf";
	return "/usr/share/fonts/noto/NotoSans-Regular.ttf";
}


void *ft_load_font(const string &name, float font_size) {
	FT_Face face = nullptr;

	//msg_write(">>> " + find_system_font_file(name).str());
	auto error = FT_New_Face(ft2, find_system_font_file(name).c_str(), 0, &face);
	if (error == FT_Err_Unknown_File_Format) {
		throw Exception("font unsupported");
	} else if (error) {
		throw Exception("font can not be loaded");
	}

	int dpi = 72;
	FT_Set_Char_Size(face, 0, int(font_size*64.0f), dpi, dpi);
	//FT_Set_Pixel_Sizes(face, (int)font_size, 0);

	return face;
}

void Font::init_fonts() {
#ifdef USE_FREETYPE
	auto error = FT_Init_FreeType(&ft2);
	if (error) {
		throw Exception("can not initialize freetype2 library");
	}
	_default = Font::load(config.get_str("default-font", "NotoSans"));
#endif
}

int ft_get_text_width_single_line(FT_Face face, const string &text) {
	auto utf32 = text.utf8_to_utf32();

	//auto glyph_index = FT_Get_Char_Index(face, 'A');
	//msg_write(glyph_index);
	//errpr = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT); //load_flags);
	//error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL); //render_mode);

	int x = 0;

#if 1
	foreachi (int u, utf32, i) {
		int error = FT_Load_Char(face, u, FT_LOAD_RENDER);
		if (error) {
			msg_error(i2s(error));
			continue;
		}
		if (i == utf32.num - 1) {
			x += max(int(face->glyph->advance.x >> 6), int(face->glyph->bitmap_left + face->glyph->bitmap.width));
		} else {
			x += face->glyph->advance.x >> 6;
		}
	}
	return x;
#else
	// less reliable

	foreachi (int u, utf32, i) {
		if (i == utf32.num - 1) {
			int error = FT_Load_Char(face, u, FT_LOAD_RENDER);
			if (error) {
				msg_error(i2s(error));
				continue;
			}
			x += max(int(face->glyph->advance.x >> 6), int(face->glyph->bitmap_left + face->glyph->bitmap.width));
		} else {
			int error = FT_Load_Glyph(face, u, FT_LOAD_DEFAULT); //load_flags);
			if (error) {
				msg_error(i2s(error));
				continue;
			}
			//wmax = max(wmax, x + face->glyph->width);
			x += face->glyph->advance.x >> 6;
		}
	}
	return x;
#endif
}

float ft_get_text_width(FT_Face face, const string &text) {
	auto lines = text.explode("\n");

	int wmax = 0;
	for (auto &l: lines) {
		wmax = max(wmax, ft_get_text_width_single_line(face, l));
	}

	return wmax;
}

void ft_render_text(Font *font, FT_Face face, const string &text, gui::Node::Align align, Image &im) {

	auto lines = text.explode("\n");

	Array<int> line_width;
	int wmax = 0;
	for (auto &l: lines) {
		int w = ft_get_text_width_single_line(face, l);
		line_width.add(w);
		wmax = max(wmax, w);
	}


	im.create(wmax, font->get_height(text), color(0,1,1,1));


	float y = face->size->metrics.ascender >> 6;//Font::FONT_SIZE * Font::LINE_Y_OFFSET;

	foreachi (auto &l, lines, line_no) {

		auto utf32 = l.utf8_to_utf32();

		int x = 0;
		if (align & Node::Align::RIGHT)
			x = wmax - line_width[line_no];

		for (int u: utf32) {
			int error = FT_Load_Char(face, u, FT_LOAD_RENDER);
			if (error)
				continue;

			for (int i=0; i<face->glyph->bitmap.width; i++)
				for (int j=0; j<face->glyph->bitmap.rows; j++) {
					float f = (float)face->glyph->bitmap.buffer[i + j*face->glyph->bitmap.width] / 255.0f;
					im.set_pixel(x+face->glyph->bitmap_left+i,y-face->glyph->bitmap_top+j, color(f, 1,1,1));
				}
			x += face->glyph->advance.x >> 6;
		}
		y += font->line_height;
	}
}

#endif

void Font::render_text(const string &str, Node::Align align, Image &im) {
#ifdef USE_CAIRO
	string font_name = "CAC Champagne";
	cairo_render_text(font_name, FONT_SIZE, str, align, im);
#else
	ft_render_text(this, (FT_Face)face, str, align, im);
#endif
}

int Font::get_width(const string &str) {
	return ft_get_text_width((FT_Face)face, str);
}

int Font::get_height(const string &str) {
	auto lines = str.explode("\n");
	return line_height * lines.num;
	return FONT_SIZE + max(lines.num - 1, 0) * line_height;
}

float Font::get_height_rel(const string &str) {
	return (float)get_height(str) / (float)line_height;//FONT_SIZE;
}

}


