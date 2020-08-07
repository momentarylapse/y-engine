/*
 * GameIni.h
 *
 *  Created on: 07.08.2020
 *      Author: michi
 */

#ifndef SRC_GAMEINI_H_
#define SRC_GAMEINI_H_

#include "lib/base/base.h"

class GameIni {
public:
	string main_script;
	string default_world;
	string second_world;
	string default_material;
	string default_font;

	void load();
};
extern GameIni game_ini;

#endif /* SRC_GAMEINI_H_ */
