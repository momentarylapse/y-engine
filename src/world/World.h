/*----------------------------------------------------------------------------*\
| God                                                                          |
| -> manages objetcs and interactions                                          |
| -> loads and stores the world data (level)                                   |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last updated: 2008.12.06 (c) by MichiSoft TM                                 |
\*----------------------------------------------------------------------------*/
#pragma once


#include "../lib/base/base.h"
#include "../lib/file/path.h"
#include "../lib/image/color.h"
#include "../y/Entity.h"
#include "LevelData.h"


class Model;
class Object;
class Material;
class Terrain;
class SolidBody;
class Entity3D;
class TemplateDataScriptVariable;
class Light;
class ParticleManager;
class Particle;
class Link;
class LevelData;
enum class LinkType;
namespace audio {
	class Sound;
}
namespace kaba {
	class Function;
}


class btDefaultCollisionConfiguration;
class btCollisionDispatcher;
class btBroadphaseInterface;
class btSequentialImpulseConstraintSolver;
class btDiscreteDynamicsWorld;



enum class PhysicsMode {
	NONE,
	SIMPLE,
	FULL_INTERNAL,
	FULL_EXTERNAL,
};


struct PartialModel {
	Model *model;
	/*vulkan::UniformBuffer *ubo;
	vulkan::DescriptorSet *dset;*/
	Material *material;
	int mat_index;
	float d;
	bool shadow, transparent;
	void clear();
};

struct PartialModelMulti {
	Model *model;
	Material *material;
	Array<matrix> matrices;
	int mat_index;
	float d;
	bool shadow, transparent;
	void clear();
};

// network messages
struct GodNetMessage {
	int msg, arg_i[4];
	string arg_s;
};

class CollisionData {
public:
	SolidBody *sb;
	Model *sub;
	Terrain *t;
	vector p, n;
};


// game data
class World {
public:
	World();
	~World();
	void reset();
	bool load(const LevelData &ld);

	Path next_filename;
	void load_soon(const Path &filename);
	void save(const Path &filename);

	Entity3D *create_entity(const vector &pos, const quaternion &ang);
	Array<Entity3D*> dummy_entities;
	void register_entity(Entity3D *e);
	void unregister_entity(Entity3D *e);

	Entity3D *create_object(const Path &filename, const vector &pos, const quaternion &ang);
	Entity3D *create_object_no_reg(const Path &filename, const vector &pos, const quaternion &ang);
	Entity3D *create_object_x(const Path &filename, const string &name, const vector &pos, const quaternion &ang, const Array<LevelData::ScriptData> &components);
	Terrain *create_terrain(const Path &filename, const vector &pos);

	Object* create_object_multi(const Path &filename, const Array<vector> &pos, const Array<quaternion> &ang);

	void register_object(Entity3D *o, int index);
	void unregister_object(Entity3D *o);
	void set_active_physics(Entity3D *o, bool active, bool passive);//, bool test_collisions);

	bool unregister(Entity *o);
	void _delete(Entity *o);

	void register_model(Model *m);
	void unregister_model(Model *m);

	void register_model_multi(Model *m, const Array<matrix> &matrices);

	void add_link(Link *l);

	Path filename;
	color background;
	Array<Model*> skybox;
	Fog fog;

	Array<Light*> lights;
	Light *add_light_parallel(const quaternion &ang, const color &c);
	Light *add_light_point(const vector &p, const color &c, float r);
	Light *add_light_cone(const vector &p, const quaternion &ang, const color &c, float r, float t);

	ParticleManager *particle_manager;
	void add_particle(Particle *p);

	void iterate(float dt);
	void iterate_physics(float dt);
	void iterate_animations(float dt);

	void shift_all(const vector &dpos);
	vector get_g(const vector &pos) const;


	float speed_of_sound;

	/*vulkan::UniformBuffer *ubo_light;
	vulkan::UniformBuffer *ubo_fog;*/

	vector gravity;


	int physics_num_steps, physics_num_link_steps;

	bool net_msg_enabled;
	Array<GodNetMessage> net_messages;

	// content of the world
	Array<Entity3D*> objects;
	Entity3D *ego;
	int num_reserved_objects;

	Array<Terrain*> terrains;

	Array<PartialModel> sorted_opaque, sorted_trans;
	Array<PartialModelMulti> sorted_multi;


	Array<LevelData::ScriptData> scripts;


	// esotherical (not in the world)
	bool add_all_objects_to_lists;


	btDefaultCollisionConfiguration* collisionConfiguration;
	btCollisionDispatcher* dispatcher;
	btBroadphaseInterface* overlappingPairCache;
	btSequentialImpulseConstraintSolver* solver;
	btDiscreteDynamicsWorld* dynamicsWorld;

	Array<Link*> links;

	PhysicsMode physics_mode;


	bool _cdecl trace(const vector &p1, const vector &p2, CollisionData &d, bool simple_test, Entity3D *o_ignore = nullptr);

	Array<audio::Sound*> sounds;
	void add_sound(audio::Sound *s);

	typedef void callback();
	struct Observer {
		string msg;
		callback *f;
	};
	void subscribe(const string &msg, kaba::Function *f);
	void notify(const string &msg);

	Array<Observer> observers;
	struct MessageData {
		Entity3D *e;
		vector v;
	} msg_data;
};
extern World world;


void GodInit();
void GodEnd();
bool GodLoadWorld(const Path &filename);


enum {
	NET_MSG_CREATE_OBJECT = 1000,
	NET_MSG_DELETE_OBJECT = 1002,
	NET_MSG_SCTEXT = 2000
};

