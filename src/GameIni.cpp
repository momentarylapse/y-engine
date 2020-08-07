/*
 * GameIni.cpp
 *
 *  Created on: 07.08.2020
 *      Author: michi
 */

#include "GameIni.h"
#include "lib/file/file.h"

GameIni game_ini;

void GameIni::load() {
	File *f = FileOpenText("game.ini");
	f->read_comment();
	main_script = f->read_str();
	f->read_comment();
	default_world = f->read_str();
	f->read_comment();
	second_world = f->read_str();
	f->read_comment();
	default_material = f->read_str();
	f->read_comment();
	default_font = f->read_str();
	delete f;
}
