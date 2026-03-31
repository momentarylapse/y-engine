/*
 * LevelData.h
 *
 *  Created on: 16.08.2020
 *      Author: michi
 */

#ifndef SRC_WORLD_LEVELDATA_H_
#define SRC_WORLD_LEVELDATA_H_


#include <lib/base/base.h>
#include <lib/os/path.h>
#include <lib/math/vec3.h>
#include <lib/image/color.h>
#include <ecs/Entity.h>

struct LevelData;


class Model;
namespace yrenderer {
	struct Material;
	enum class LightType;
}
class Terrain;
struct ScriptInstanceData;
struct Light;
class ParticleManager;
struct Particle;


enum class PhysicsMode;
enum class LinkType;

struct Fog {
	bool enabled;
	int mode;
	float start, end, distance;
	color _color;
};


struct LevelData {
	LevelData();
	bool load(const Path &filename);
	void save(const Path &filename);

	static Array<ScriptInstanceData> auto_terrain_components();

	struct Entity {
		string name;
		vec3 pos;
		quaternion ang;
		Array<ScriptInstanceData> components;
	};

	Path world_filename;
	Array<Path> skybox_filename;
	Array<vec3> skybox_ang;
	color background_color;
	Array<Entity> entities;
	Array<ScriptInstanceData> systems;

	bool physics_enabled;
	PhysicsMode physics_mode;
	vec3 gravity;
	Fog fog;
};



#endif /* SRC_WORLD_LEVELDATA_H_ */
