/*
 * Config.h
 *
 *  Created on: 07.08.2020
 *      Author: michi
 */

#ifndef SRC_CONFIG_H_
#define SRC_CONFIG_H_


#include "lib/hui_minimal/Config.h"

#include "lib/base/base.h"

class Config : public hui::Configuration {
public:
	bool debug = false;
	string main_script;
	string default_world;
	string second_world;
	string default_material;
	string default_font;

	Config();
	void load();
};
extern Config config;

#endif /* SRC_CONFIG_H_ */
