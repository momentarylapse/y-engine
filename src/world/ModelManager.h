/*
 * ModelManager.h
 *
 *  Created on: 19.01.2020
 *      Author: michi
 */

#pragma once

#include "../lib/base/base.h"

class Model;
class Path;


class ModelManager {
public:
	static Model *load(const Path &filename);

	static Array<Model*> originals;
};
