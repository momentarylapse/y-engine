/*
 * PluginManager.h
 *
 *  Created on: 02.01.2020
 *      Author: michi
 */

#ifndef SRC_PLUGINS_PLUGINMANAGER_H_
#define SRC_PLUGINS_PLUGINMANAGER_H_

#include "../lib/base/base.h"

class Controller;

class PluginManager {
public:
	static void link_kaba();

	void reset();

	static void *create_instance(const string &filename, const string &base_class);

	void add_controller(const string &name);

	Array<Controller*> controllers;
};


extern PluginManager plugin_manager;

#endif /* SRC_PLUGINS_PLUGINMANAGER_H_ */
