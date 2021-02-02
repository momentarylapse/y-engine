/*
 * Text.h
 *
 *  Created on: 04.01.2020
 *      Author: michi
 */

#ifndef SRC_GUI_TEXT_H_
#define SRC_GUI_TEXT_H_

#include "Picture.h"

namespace gui {

class Font;

class Text : public Picture {
public:
	Text(const string &t, float h, float x, float y);
	~Text() override;
	void __init2__(const string &t, float h);
	void __init4__(const string &t, float h, float x, float y);
	void __delete__() override;

	void rebuild();
	void set_text(const string &t);

	string text;
	float font_size;
	Font *font;
};

}

#endif /* SRC_GUI_TEXT_H_ */
