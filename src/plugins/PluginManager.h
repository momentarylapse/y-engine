/*
 * PluginManager.h
 *
 *  Created on: 02.01.2020
 *      Author: michi
 */

#ifndef SRC_PLUGINS_PLUGINMANAGER_H_
#define SRC_PLUGINS_PLUGINMANAGER_H_

#include "../lib/base/base.h"

class Path;
class Controller;
class PerformanceMonitor;
class TemplateDataScriptVariable;

class PluginManager {
public:
	static void link_kaba();

	void reset();

	static void *create_instance(const Path &filename, const string &base_class, Array<TemplateDataScriptVariable> &variables);

	void add_controller(const Path &name, Array<TemplateDataScriptVariable> &variables);

	Array<Controller*> controllers;

	void handle_iterate_pre(float dt);
	void handle_iterate(float dt);
	void handle_input();
	void handle_draw_pre();
};


extern PluginManager plugin_manager;
extern PerformanceMonitor *global_perf_mon;

#endif /* SRC_PLUGINS_PLUGINMANAGER_H_ */
