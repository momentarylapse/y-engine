/*
 * GameIni.cpp
 *
 *  Created on: 07.08.2020
 *      Author: michi
 */

#include "GameIni.h"
#include "lib/hui/Config.h"

GameIni game_ini;

void GameIni::load() {
	hui::Configuration conf("game.ini");
	conf.load();
	default_world = conf.get_str("default-world", "");
	second_world = conf.get_str("second-world", "");
	main_script = conf.get_str("main-script", "");
	default_font = conf.get_str("default-font", "");
	default_material = conf.get_str("default-material", "");
}
