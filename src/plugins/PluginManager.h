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
namespace kaba {
	class Class;
}

class PluginManager {
public:
	static void init();
	static void export_kaba();
	static void import_kaba();

	static void reset();

	static const kaba::Class *find_class(const Path &filename, const string &name);
	static const kaba::Class *find_class_derived(const Path &filename, const string &base_class);
	static void *create_instance(const kaba::Class *type, const string &variables);
	static void *create_instance(const kaba::Class *type, const Array<TemplateDataScriptVariable> &variables);
	static void *create_instance(const Path &filename, const string &base_class, const Array<TemplateDataScriptVariable> &variables);
	static void assign_variables(void *p, const kaba::Class *c, const Array<TemplateDataScriptVariable> &variables);
	static void assign_variables(void *p, const kaba::Class *c, const string &variables);

	static void add_controller(const Path &name, const Array<TemplateDataScriptVariable> &variables);
	static Controller *get_controller(const kaba::Class *_class);

	static Array<Controller*> controllers;
};

#endif /* SRC_PLUGINS_PLUGINMANAGER_H_ */
