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

enum class AntialiasingMethod {
	NONE,
	MSAA,
	TAA
};

class Config : public hui::Configuration {
public:
	bool debug = false;
	string main_script;
	string default_world;
	string second_world;
	string default_material;
	string default_font;
	AntialiasingMethod antialiasing_method = AntialiasingMethod::NONE;

	float resolution_scale_min = 0.5f;
	float resolution_scale_max = 1;
	float target_framerate = 60;

	Config();
	void load(const Array<string> &arg);
};
extern Config config;

#endif /* SRC_CONFIG_H_ */
