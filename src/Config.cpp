/*
 * Config.cpp
 *
 *  Created on: 07.08.2020
 *      Author: michi
 */

#include "Config.h"
#include "lib/file/file.h"

Config config;

void Config::load() {
	File *f = FileOpenText("config.txt");
	while(!f->end()) {
		string s = f->read_str();
		if (s.num == 0)
			continue;
		if (s[0] == '#')
			continue;
		int p = s.find("=");
		if (p >= 0) {
			map.set(s.head(p).replace(" ", ""), s.substr(p+1, -1).replace(" ", ""));
		}
	}
	for (auto &k: map.keys())
		msg_write("config:  " + k + " == " + map[k]);
	delete f;
	debug = get_bool("debug", false);
}
string Config::get(const string &key, const string &def) {
	if (map.find(key) >= 0)
		return map[key];
	return def;
}
bool Config::get_bool(const string &key, bool def) {
	string s = get(key, def ? "yes" : "no");
	return s == "yes" or s == "true" or s == "1";
}
float Config::get_float(const string &key, float def) {
	return get(key, f2s(def, 3))._float();
}
int Config::get_int(const string &key, int def) {
	return get(key, i2s(def))._int();
}
