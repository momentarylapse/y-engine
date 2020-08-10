/*
 * Config.cpp
 *
 *  Created on: 07.08.2020
 *      Author: michi
 */

#include "Config.h"
#include "lib/file/file.h"

Config config;

Config::Config() {
}

void Config::load() {
	hui::Configuration::load("game.ini");

	default_world = get_str("default-world", "");
	second_world = get_str("second-world", "");
	main_script = get_str("main-script", "");
	default_font = get_str("default-font", "");
	default_material = get_str("default-material", "");
	debug = get_bool("debug", true);
}