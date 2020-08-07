/*
 * Config.h
 *
 *  Created on: 07.08.2020
 *      Author: michi
 */

#ifndef SRC_CONFIG_H_
#define SRC_CONFIG_H_


#include "lib/base/base.h"
#include "lib/base/map.h"

class Config {
public:
	bool debug = false;
	Map<string,string> map;
	void load();
	string get(const string &key, const string &def);
	bool get_bool(const string &key, bool def);
	float get_float(const string &key, float def);
	int get_int(const string &key, int def);
};
extern Config config;

#endif /* SRC_CONFIG_H_ */
