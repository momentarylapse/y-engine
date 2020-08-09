/*
 * Config.cpp
 *
 *  Created on: 07.08.2020
 *      Author: michi
 */

#include "Config.h"
#include "lib/file/file.h"

Config config;

Config::Config() : hui::Configuration("config.txt") {
}

void Config::load() {
	hui::Configuration::load();
	debug = get_bool("debug", true);
}
