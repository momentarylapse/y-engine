//
// Created by Michael Ankele on 2024-10-13.
//

#ifndef CONTROLLERMANAGER_H
#define CONTROLLERMANAGER_H

#include "../lib/base/base.h"

class Path;
class Controller;
class TemplateDataScriptVariable;
namespace kaba {
	class Class;
}

class ControllerManager {
public:
	static void init(int ch_iter);

	static void reset();

	static void add_controller(const Path &name, const Array<TemplateDataScriptVariable> &variables);
	static Controller *get_controller(const kaba::Class *_class);

	static Array<Controller*> controllers;
};



#endif //CONTROLLERMANAGER_H
