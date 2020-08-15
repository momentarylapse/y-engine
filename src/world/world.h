/*----------------------------------------------------------------------------*\
| God                                                                          |
| -> manages objetcs and interactions                                          |
| -> loads and stores the world data (level)                                   |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last updated: 2008.12.06 (c) by MichiSoft TM                                 |
\*----------------------------------------------------------------------------*/
#if !defined(GOD_H__INCLUDED_)
#define GOD_H__INCLUDED_


#include "../lib/base/base.h"
#include "../lib/file/path.h"
#include "../lib/math/math.h"


class Model;
class Object;
class Material;
class Terrain;
class TemplateDataScriptVariable;
class Light;
class ParticleManager;
class Particle;


class btDefaultCollisionConfiguration;
class btCollisionDispatcher;
class btBroadphaseInterface;
class btSequentialImpulseConstraintSolver;
class btDiscreteDynamicsWorld;
class btTypedConstraint;


class Fog {
public:
	bool enabled;
	int mode;
	float start, end, distance;
	color _color;
};

enum class LinkType {
	SOCKET,
	HINGE,
	UNIVERSAL,
	SPRING,
	SLIDER
};

enum class PhysicsMode {
	NONE,
	SIMPLE,
	FULL_INTERNAL,
	FULL_EXTERNAL,
};

class LevelData {
public:
	LevelData();
	bool load(const Path &filename);


	class Terrain {
	public:
		Path filename;
		vector pos;
	};

	class Object {
	public:
		Path filename;
		string name;
		vector pos, ang, vel, rot;
	};

	class Light {
	public:
		bool enabled;
		vector pos, ang;
		color _color;
		float radius, harshness;
	};

	class Camera {
	public:
		vector pos, ang;
		float fov, min_depth, max_depth, exposure;
	};

	class Script {
	public:
		Path filename;
		Array<TemplateDataScriptVariable> variables;
	};

	class Link {
	public:
		int object[2];
		LinkType type;
		vector pos, ang;
	};

	Path world_filename;
	Array<Path> skybox_filename;
	Array<vector> skybox_ang;
	color background_color;
	Array<Object> objects;
	Array<Terrain> terrains;
	int ego_index;
	Array<Script> scripts;
	Array<Light> lights;
	Array<Link> links;

	Array<Camera> cameras;

	bool physics_enabled;
	PhysicsMode physics_mode;
	vector gravity;
	Fog fog;
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

// network messages
struct GodNetMessage {
	int msg, arg_i[4];
	string arg_s;
};


class Link {
public:
	Link(LinkType type, Object *a, Object *b, const vector &pos, const quaternion &ang);
	~Link();

	void set_motor(float v, float max);
	void set_frame(int n, const quaternion &q);

	btTypedConstraint *con;
	LinkType type;
	Object *a;
	Object *b;
};



// game data
class World {
public:
	World();
	~World();
	void reset();
	bool load(const LevelData &ld);
	Object *create_object(const Path &filename, const string &name, const vector &pos, const quaternion &ang, int w_index = -1);
	Terrain *create_terrain(const Path &filename, const vector &pos);

	void register_object(Object *o, int index);
	void unregister_object(Object *o);

	void register_model(Model *m);
	void unregister_model(Model *m);

	Link *add_link(LinkType type, Object *a, Object *b, const vector &pos, const quaternion &ang);

	Path filename;
	color background;
	Array<Model*> skybox;
	Fog fog;

	Array<Light*> lights;
	void add_light(Light *l);

	ParticleManager *particle_manager;
	void add_particle(Particle *p);

	void iterate(float dt);

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
	Array<Object*> objects;
	Object *ego;
	Object *terrain_object;
	int num_reserved_objects;

	Array<Terrain*> terrains;

	Array<PartialModel> sorted_opaque, sorted_trans;


	Array<LevelData::Script> scripts;


	// esotherical (not in the world)
	bool add_all_objects_to_lists;


	btDefaultCollisionConfiguration* collisionConfiguration;
	btCollisionDispatcher* dispatcher;
	btBroadphaseInterface* overlappingPairCache;
	btSequentialImpulseConstraintSolver* solver;
	btDiscreteDynamicsWorld* dynamicsWorld;

	Array<Link*> links;

	PhysicsMode physics_mode;
};
extern World world;


void GodInit();
void GodEnd();
bool GodLoadWorld(const Path &filename);

void AddNewForceField(vector pos,vector dir,int kind,int shape,float r,float v,float a,bool visible,float t);
//void DoSounds();
void SetSoundState(bool paused,float scale,bool kill,bool restart);
void GodCalcMove(float dt);
void GodDoAnimation(float dt); // debug
void GodIterateObjects(float dt);
void GodDoCollisionDetection();
Object *_cdecl GetObjectByName(const string &name);
void _cdecl GodObjectEnsureExistence(int id);
int _cdecl GodFindObjects(vector &pos, float radius, int mode, Array<Object*> &a);

void Test4Ground(Object *o);
void Test4Object(Object *o1,Object *o2);


// what is hit (TraceData.type)
enum {
	TRACE_TYPE_NONE = -1,
	TRACE_TYPE_TERRAIN,
	TRACE_TYPE_MODEL
};

class TraceData {
public:
	int type;
	vector point;
	Terrain *terrain;
	Model *model;
	Model *object;
};
bool _cdecl GodTrace(const vector &p1, const vector &p2, TraceData &d, bool simple_test, Model *o_ignore = NULL);



/*#define FFKindRadialConst		0
#define FFKindRadialLinear		1
#define FFKindRadialQuad		2
#define FFKindDirectionalConst	10
#define FFKindDirectionalLinear	11
#define FFKindDirectionalQuad	12*/

enum {
	NET_MSG_CREATE_OBJECT = 1000,
	NET_MSG_DELETE_OBJECT = 1002,
	NET_MSG_SCTEXT = 2000
};

#endif
