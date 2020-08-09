/*
 * Config.h
 *
 *  Created on: 07.08.2020
 *      Author: michi
 */

#ifndef SRC_CONFIG_H_
#define SRC_CONFIG_H_


#include "lib/base/base.h"
#include "lib/hui/Config.h"

class Config : public hui::Configuration {
public:
	bool debug = false;

	Config();
	void load();
};
extern Config config;

#endif /* SRC_CONFIG_H_ */
