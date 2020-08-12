/*
 * ModelManager.cpp
 *
 *  Created on: 19.01.2020
 *      Author: michi
 */

#include "ModelManager.h"
#include "model.h"
#include "../meta.h"
#include "../lib/kaba/kaba.h"
#include "../plugins/PluginManager.h"

Array<Model*> ModelManager::originals;

Model* fancy_copy(Model *m) {
	Model *c = nullptr;
	//msg_write(format("MODEL  %s   %s", m->_template->filename, m->_template->script_filename));
	if (!m->_template->script_filename.is_empty())
		c = (Model*)plugin_manager.create_instance(m->_template->script_filename, "y.Model", m->_template->variables);
	if (!c)
		c = new Model();
	return m->copy(c);
}

Model* ModelManager::load(const Path &_filename) {
	auto filename = engine.object_dir << (_filename.with(".model"));
	for (auto *o: originals)
		if (o->_template->filename == filename) {
			return fancy_copy(o);
		}

	Model *m = new Model();
	m->load(filename);
	originals.add(m);
	return fancy_copy(m);
}
