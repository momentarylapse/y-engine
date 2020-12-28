/*
 * ModelManager.cpp
 *
 *  Created on: 19.01.2020
 *      Author: michi
 */

#include "ModelManager.h"
#include "Model.h"
#include "../y/EngineData.h"
#include "../lib/kaba/kaba.h"
#include "../plugins/PluginManager.h"

Array<Model*> ModelManager::originals;

Model* fancy_copy(Model *m, const Path &_script) {
	Model *c = nullptr;
	Path script = m->_template->script_filename;
	if (!_script.is_empty())
		script = _script;
	//msg_write(format("MODEL  %s   %s", m->_template->filename, script));
	if (!script.is_empty())
		c = (Model*)plugin_manager.create_instance(script, "y.Model", m->_template->variables);
	if (!c)
		c = new Model();
	return m->copy(c);
}

Model* ModelManager::load(const Path &_filename) {
	return loadx(_filename, "");
}

Model* ModelManager::loadx(const Path &_filename, const Path &_script) {
	auto filename = engine.object_dir << _filename.with(".model");
	for (auto *o: originals)
		if (o->_template->filename == filename) {
			return fancy_copy(o, _script);
		}

	Model *m = new Model();
	m->load(filename);
	originals.add(m);
	return fancy_copy(m, _script);
}
