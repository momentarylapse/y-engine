/*
 * ModelManager.h
 *
 *  Created on: 19.01.2020
 *      Author: michi
 */

#pragma once

#include <lib/base/base.h>
#include <lib/base/pointer.h>
#include <lib/os/path.h>

class MetaMove;
class Model;
class Path;
class SolidBody;
class MeshCollider;
class Animator;
class Skeleton;
struct ScriptInstanceData;
class ResourceManager;
namespace yrenderer {
	class MaterialManager;
}


class ModelTemplate : public Sharable<base::Empty> {
public:
	Path filename;
	Model *model;
	Array<Path> bone_model_filename;
	SolidBody *solid_body;
	MeshCollider *mesh_collider;
	//Animator *animator;
	shared<MetaMove> meta_move;
	Skeleton *skeleton;
	Array<ScriptInstanceData> components;


	explicit ModelTemplate(Model *m);
};

class ModelManager {
public:
	ModelManager(ResourceManager *resource_manager, yrenderer::MaterialManager *material_manager);
	xfer<Model> load(const Path &filename);

	ResourceManager *resource_manager;
	yrenderer::MaterialManager *material_manager;
	Array<Model*> originals;
};
