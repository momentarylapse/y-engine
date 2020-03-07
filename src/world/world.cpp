/*----------------------------------------------------------------------------*\
| God                                                                          |
| -> manages objetcs and interactions                                          |
| -> loads and stores the world data (level)                                   |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last updated: 2009.11.22 (c) by MichiSoft TM                                 |
\*----------------------------------------------------------------------------*/

#include <algorithm>
#include "world.h"
#include "../lib/file/file.h"
//#include "../lib/vulkan/vulkan.h"
#include "../lib/nix/nix.h"
#include "../meta.h"
#include "object.h"
#include "model.h"
#include "ModelManager.h"
#include "material.h"
#include "terrain.h"

#include "../lib/xfile/xml.h"

#ifdef _X_ALLOW_X_
#include "../fx/Light.h"
#include "../fx/Particle.h"
#endif

#if 0
#include "model_manager.h"
#include "../lib/nix/nix.h"
#ifdef _X_ALLOW_X_
#include "../physics/physics.h"
#include "../physics/links.h"
#include "../physics/collision.h"
#include "../fx/fx.h"
#endif
#include "../networking.h"
#endif
#include "camera.h"


nix::Texture *tex_white = nullptr;
nix::Texture *tex_black = nullptr;


//vulkan::DescriptorSet *rp_create_dset(const Array<vulkan::Texture*> &tex, vulkan::UniformBuffer *ubo);



//#define _debug_matrices_


#ifdef USE_ODE
	#define dSINGLE
	#include <ode/ode.h>
	bool ode_world_created = false;
	dWorldID world_id;
	dSpaceID space_id;
	dJointGroupID contactgroup;
inline void qx2ode(quaternion *qq, dQuaternion q)
{
	q[0] = qq->w;
	q[1] = qq->x;
	q[2] = qq->y;
	q[3] = qq->z;
}
inline void qode2x(const dQuaternion q, quaternion *qq)
{
	qq->w = q[0];
	qq->x = q[1];
	qq->y = q[2];
	qq->z = q[3];
}
#endif


#ifdef _X_ALLOW_PHYSICS_DEBUG_
	int PhysicsTimer;
	float PhysicsTimeCol, PhysicsTimePhysics, PhysicsTimeLinks;
	bool PhysicsStopOnCollision = false;
#endif



// game data
World world;


#ifdef _X_ALLOW_X_
void DrawSplashScreen(const string &str, float per);
void ScriptingObjectInit(Object *o);
#else
void DrawSplashScreen(const string &str, float per){}
void ScriptingObjectInit(Object *o){}
#endif


// network messages
void AddNetMsg(int msg, int argi0, const string &args)
{
#if 0
#ifdef _X_ALLOW_X_
	if ((!world.net_msg_enabled) || (!Net.Enabled))
		return;
	GodNetMessage m;
	m.msg = msg;
	m.arg_i[0] = argi0;
	m.arg_s = args;
	world.net_messages.add(m);
#endif
#endif
}


int num_insane=0;

inline bool TestVectorSanity(vector &v, const char *name)
{
	if (inf_v(v)){
		num_insane++;
		v=v_0;
		if (num_insane>100)
			return false;
		msg_error(format("Vektor %s unendlich!!!!!!!",name));
		return true;
	}
	return false;
}

void TestObjectSanity(const char *str)
{
#ifdef _X_ALLOW_PHYSICS_DEBUG_
	for (int i=0;i<Objects.num;i++)
		if (Objects[i]){
			Object *o=Objects[i];
			/*if (((int)o>1500000000)||((int)o<10000000)){
				msg_write(str);
				msg_error("Objekt-Pointer kaputt!");
				msg_write(i);
				msg_write((int)o);
			}*/
			bool e=false;
			e|=TestVectorSanity(o->Pos,"Pos");
			e|=TestVectorSanity(o->Vel,"Vel");
			e|=TestVectorSanity(o->ForceExt,"ForceExt");
			e|=TestVectorSanity(o->TorqueExt,"TorqueExt");
			e|=TestVectorSanity(o->ForceInt,"ForceInt");
			e|=TestVectorSanity(o->TorqueInt,"TorqueInt");
			e|=TestVectorSanity(o->Ang,"Ang");
			e|=TestVectorSanity(o->Rot,"Rot");
			if (e){
				msg_write(string2("%s:  objekt[%d] (%s) unendlich!!!!",str,i,o->Name));
				HuiRaiseError("Physik...");
			}
		}
#endif
}







void GodInit() {
//	world.ubo_light = new vulkan::UniformBuffer(sizeof(UBOLight), 64);
//	world.ubo_fog = new vulkan::UniformBuffer(64);

	Image im;
	tex_white = new nix::Texture();
	tex_black = new nix::Texture();
	im.create(16, 16, White);
	tex_white->overwrite(im);
	im.create(16, 16, Black);
	tex_black->overwrite(im);

	world.reset();

	world.terrain_object = new Object();
	world.terrain_object->update_matrix();

#if 0
	COctree *octree = new COctree(v_0, 100);
	sOctreeLocationData dummy_loc;
	vector min =vector(23,31,9);
	vector max =vector(40,50,39);
	octree->Insert(min, max, (void*)1, &dummy_loc);
	min =vector(23,31,9);
	max =vector(24,32,10);
	octree->Insert(min, max, (void*)2, &dummy_loc);

	Array<void*> a;
	vector pos = vector(24, 30, 20);
	octree->GetPointNeighbourhood(pos, 100, a);

	msg_write("---Octree-Test---");
	msg_write(a.num);
	for (int i=0;i<a.num;i++)
		msg_write(p2s(a[i]));
	//exit(0);
	msg_write("-----------------");
#endif

	
	

#ifdef USE_ODE
	contactgroup = dJointGroupCreate(0);
#endif
}

void GodEnd() {
//	delete world.ubo_light;
//	delete world.ubo_fog;
}

World::World() {
//	ubo_light = nullptr;
//	ubo_fog = nullptr;

	world.particle_manager = new ParticleManager();

	reset();
}

void World::reset() {
	net_msg_enabled = false;
	net_messages.clear();

	gravity = v_0;

	// terrains
	for (auto *t: terrains)
		delete t;
	terrains.clear();

	// objects
	for (auto *o: objects)
		if (o)
			delete o;//unregister_object(o); // actual deleting done by ModelManager
	objects.clear();
	num_reserved_objects = 0;
	
	for (auto &s: sorted_trans)
		s.clear();
	sorted_trans.clear();
	for (auto &s: sorted_opaque)
		s.clear();
	sorted_opaque.clear();

	for (auto *l: lights)
		delete l;
	lights.clear();

	particle_manager->clear();



	// music
	/*if (meta->MusicEnabled){
		NixSoundStop(MusicCurrent);
	}*/

	// skybox
	//   (models deleted by meta)
	skybox.clear();
	

	// initial data for empty world...
	fog._color = White;
	fog.mode = 0;//FOG_EXP;
	fog.distance = 10000;
	fog.enabled = false;
	fog.start = 0;
	fog.end = 100000;
	speed_of_sound = 1000;
	
	engine.physics_enabled = false;
	engine.collisions_enabled = true;
	physics_num_steps = 10;
	physics_num_link_steps = 5;


	// physics
#ifdef _X_ALLOW_X_
	//LinksReset();
#endif
#ifdef USE_ODE
	if (ode_world_created){
		dWorldDestroy(world_id);
		dSpaceDestroy(space_id);
	}else{
		dInitODE();
	}
	world_id = dWorldCreate();
	space_id = dSimpleSpaceCreate(0);
	//space_id = dHashSpaceCreate(0);
	/*int m1, m2;
	dHashSpaceGetLevels(space_id, &m1, &m2);
	printf("hash:    %d  %d\n", m1, m2);*/
	ode_world_created = true;
	world.terrain_object->body_id = 0;
	world.terrain_object->geom_id = dCreateSphere(0, 1); // space, radius
	dGeomSetBody((dGeomID)world.terrain_object->geom_id, (dBodyID)world.terrain_object->body_id);
#endif
}

void LevelData::reset() {
	world_filename = "";
	terrains.clear();
	objects.clear();
	skybox_filename.clear();
	skybox_ang.clear();
	scripts.clear();

	ego_index = -1;
	background_color = Gray;
	lights.clear();

	gravity = v_0;
	fog.enabled = false;
}

color ReadColor3(File *f) {
	int c[3];
	for (int i=0;i<3;i++)
		c[i] = f->read_float();
	return ColorFromIntRGB(c);
}

color ReadColor4(File *f) {
	int c[4];
	for (int i=0;i<4;i++)
		c[i] = f->read_float();
	return ColorFromIntARGB(c);
}

bool World::load(const LevelData &ld) {
	net_msg_enabled = false;
	bool ok = true;
	reset();


	engine.physics_enabled = ld.physics_enabled;
	engine.collisions_enabled = true;//LevelData.physics_enabled;
	gravity = ld.gravity;
	fog = ld.fog;

#ifdef _X_ALLOW_X_
	for (auto &l: ld.lights) {
		auto *ll = new Light(l.pos, l.ang.ang2dir(), l._color, l.radius, -1);
		ll->harshness = l.harshness;
		ll->enabled = l.enabled;
		add_light(ll);
	}
#endif

	// skybox
	skybox.resize(ld.skybox_filename.num);
	for (int i=0; i<skybox.num; i++){
		skybox[i] = ModelManager::load(ld.skybox_filename[i]);
		if (skybox[i])
			skybox[i]->ang = quaternion::rotation_v(ld.skybox_ang[i]);
	}
	background = ld.background_color;

	for (auto &c: ld.cameras) {
		cam->pos = c.pos;
		cam->ang = quaternion::rotation(c.ang);
		cam->min_depth = c.min_depth;
		cam->max_depth = c.max_depth;
		cam->exposure = c.exposure;
		cam->fov = c.fov;
		break;
	}

	// objects
	ego = NULL;
	objects.clear(); // make sure the "missing" objects are NULL
	objects.resize(ld.objects.num);
	num_reserved_objects = ld.objects.num;
	foreachi(auto &o, ld.objects, i)
		if (o.filename.num > 0){
			auto q = quaternion::rotation(o.ang);
			Object *oo = create_object(o.filename, o.name, o.pos, q, i);
			ok &= (oo >= 0);
			if (oo){
				oo->vel = o.vel;
				oo->rot = o.rot;
			}
			if (ld.ego_index == i)
				ego = oo;
			if (i % 5 == 0)
				DrawSplashScreen("Objects", (float)i / (float)ld.objects.num / 5 * 3);
		}
	add_all_objects_to_lists = true;

	// terrains
	foreachi(auto &t, ld.terrains, i){
		DrawSplashScreen("Terrain...", 0.6f + (float)i / (float)ld.terrains.num * 0.4f);
		Terrain *tt = create_terrain(t.filename, t.pos);
		ok &= !tt->error;
	}

	scripts = ld.scripts;

	net_msg_enabled = true;
	return ok;
}

Terrain *World::create_terrain(const string &filename, const vector &pos) {
	Terrain *tt = new Terrain(filename, pos);

//	tt->ubo = new vulkan::UniformBuffer(64*3);
//	tt->dset = rp_create_dset(tt->material->textures, tt->ubo);
	terrains.add(tt);
	return tt;
}

static vector s2v(const string &s) {
	auto x = s.explode(" ");
	return vector(x[0]._float(), x[1]._float(), x[2]._float());
}

// RGBA
static color s2c(const string &s) {
	auto x = s.explode(" ");
	return color(x[3]._float(), x[0]._float(), x[1]._float(), x[2]._float());
}

bool LevelData::load(const string &filename) {
	world_filename = filename;

	xml::Parser p;
	p.load(engine.map_dir + filename + ".world");
	auto *meta = p.elements[0].find("meta");
	if (meta) {
		for (auto &e: meta->elements) {
			if (e.tag == "background") {
				background_color = s2c(e.value("color"));
			} else if (e.tag == "skybox") {
				skybox_filename.add(e.value("file"));
				skybox_ang.add(v_0);
			} else if (e.tag == "physics") {
				physics_enabled = e.value("enabled")._bool();
				gravity = s2v(e.value("gravity"));
			} else if (e.tag == "fog") {
				fog.enabled = e.value("enabled")._bool();
				fog.mode = e.value("mode")._int();
				fog.start = e.value("start")._float();
				fog.end = e.value("end")._float();
				fog.distance = 1.0f / e.value("density")._float();
				fog._color = s2c(e.value("color"));
			} else if (e.tag == "script") {
				LevelDataScript s;
				s.filename = e.value("file");
				for (auto &ee: e.elements) {
					TemplateDataScriptVariable v;
					v.name = ee.value("name").lower().replace("_", "");
					v.value = ee.value("value");
					s.variables.add(v);
				}
				scripts.add(s);
			}
		}
	}


	auto *cont = p.elements[0].find("3d");
	if (cont) {
		for (auto &e: cont->elements) {
			if (e.tag == "camera") {
				LevelDataCamera c;
				c.pos = s2v(e.value("pos"));
				c.ang = s2v(e.value("ang"));
				c.fov = e.value("fov", f2s(pi/4, 3))._float();
				c.min_depth = e.value("minDepth", "1")._float();
				c.max_depth = e.value("maxDepth", "10000")._float();
				c.exposure = e.value("exposure", "1")._float();
				cameras.add(c);
			} else if (e.tag == "light") {
				LevelDataLight l;
				l.radius = e.value("radius")._float();
				l.harshness = e.value("harshness")._float();
				l._color = s2c(e.value("color"));
				l.ang = s2v(e.value("ang"));
				if (e.value("type") == "directional")
					l.radius = -1;
				l.enabled = e.value("enabled", "true")._bool();
				lights.add(l);
			} else if (e.tag == "terrain") {
				LevelDataTerrain t;
				t.filename = e.value("file");
				t.pos = s2v(e.value("pos"));
				terrains.add(t);
			} else if (e.tag == "object") {
				LevelDataObject o;
				o.filename = e.value("file");
				o.name = e.value("name");
				o.pos = s2v(e.value("pos"));
				o.ang = s2v(e.value("ang"));
				o.vel = v_0;
				o.rot = v_0;
				objects.add(o);
			}
		}
	}

	return true;
}

bool GodLoadWorld(const string &filename) {
	LevelData level_data;
	bool ok = level_data.load(filename);
	ok &= world.load(level_data);
	return ok;
}


Object *World::create_object(const string &filename, const string &name, const vector &pos, const quaternion &ang, int w_index) {
	if (engine.resetting_game)
		throw Exception("CreateObject during game reset");

	if (filename == "")
		throw Exception("CreateObject: empty filename");

	//msg_write(on);
	Model *m = ModelManager::load(filename);

	Object *o = (Object*)m;
	m->script_data.name = name;
	m->pos = pos;
	m->ang = ang;
	o->update_matrix();
	o->update_theta();


	register_object(m, w_index);

	m->on_init();

	AddNetMsg(NET_MSG_CREATE_OBJECT, m->object_id, filename);

	return o;
}

void World::register_object(Model *o, int index) {
	int on = index;
	if (on < 0){
		// ..... better use a list of "empty" objects???
		for (int i=num_reserved_objects; i<objects.num; i++)
			if (!objects[i])
				on = i;
	}else{
		if (on >= objects.num)
			objects.resize(on+1);
		if (objects[on]){
			msg_error("CreateObject:  object index already in use " + i2s(on));
			return;
		}
	}
	if (on < 0){
		on = objects.num;
		objects.add(NULL);
	}
	objects[on] = (Object*)o;

	register_model(o);

	o->object_id = on;

#ifdef USE_ODE
	if (o->physics_data.active){
		o->body_id = dBodyCreate(world_id);
		dBodySetPosition((dBodyID)o->body_id, o->pos.x, o->pos.y, o->pos.z);
		dMass m;
		dMassSetParameters(&m, o->physics_data.mass, 0, 0, 0, o->physics_data.theta._00, o->physics_data.theta._11, o->physics_data.theta._22, o->physics_data.theta._01, o->physics_data.theta._02, o->physics_data.theta._12);
		dBodySetMass((dBodyID)o->body_id, &m);
		dBodySetData((dBodyID)o->body_id, o);
		dQuaternion qq;
		qx2ode(&o->ang, qq);
		dBodySetQuaternion((dBodyID)o->body_id, qq);
	}else
		o->body_id = 0;

	vector d = o->prop.max - o->prop.min;
	o->geom_id = dCreateBox(space_id, d.x, d.y, d.z); //dCreateSphere(0, 1); // space, radius
//	msg_write((int)o->geom_id);
	dGeomSetBody((dGeomID)o->geom_id, (dBodyID)o->body_id);
#endif
}



// un-object a model
void World::unregister_object(Model *m) {
	if (m->object_id < 0)
		return;

#ifdef USE_ODE
	if (m->body_id != 0)
		dBodyDestroy((dBodyID)m->body_id);
	m->body_id = 0;

	if (m->geom_id != 0)
		dGeomDestroy((dGeomID)m->geom_id);
	m->geom_id = 0;
#endif

	// ego...
	if (m == ego)
		ego = NULL;

	AddNetMsg(NET_MSG_DELETE_OBJECT, m->object_id, "");

	// remove from list
	objects[m->object_id] = NULL;
	m->object_id = -1;
}

void PartialModel::clear() {
//	delete ubo;
//	delete dset;
}

// add a model to the (possible) rendering list
void World::register_model(Model *m) {
	if (m->registered)
		return;
	
	for (int i=0;i<m->material.num;i++){
		Material *mat = m->material[i];
		bool trans = false;//!mat->alpha.z_buffer; //false;
		/*if (mat->TransparencyMode>0){
			if (mat->TransparencyMode == TransparencyModeFunctions)
				trans = true;
			if (mat->TransparencyMode == TransparencyModeFactor)
				trans = true;
		}*/

		PartialModel p;
		p.model = m;
		p.material = mat;
//		p.ubo = new vulkan::UniformBuffer(64*3);
//		p.dset = rp_create_dset(mat->textures, p.ubo);
		p.mat_index = i;
		p.transparent = trans;
		p.shadow = false;
		if (trans)
			sorted_trans.add(p);
		else
			sorted_opaque.add(p);
	}

#ifdef _X_ALLOW_FX_
	for (int i=0;i<m->fx.num;i++)
		if (m->fx[i])
			m->fx[i]->enable(true);
#endif
	
	m->registered = true;
	
	// sub models
	for (int i=0;i<m->bone.num;i++)
		if (m->bone[i].model)
			register_model(m->bone[i].model);
}

// remove a model from the (possible) rendering list
void World::unregister_model(Model *m) {
	if (!m->registered)
		return;
	//printf("%p   %s\n", m, MetaGetModelFilename(m));

	foreachi (auto &s, sorted_trans, i)
		if (s.model == m) {
			s.clear();
			sorted_trans.erase(i);
		}
	foreachi (auto &s, sorted_opaque, i)
		if (s.model == m) {
			s.clear();
			sorted_opaque.erase(i);
		}

#ifdef _X_ALLOW_FX_
	if (!engine.resetting_game)
		for (int i=0;i<m->fx.num;i++)
			if (m->fx[i])
				m->fx[i]->enable(false);
#endif
	
	m->registered = false;
	//printf("%d\n", m->NumBones);

	// sub models
	for (int i=0;i<m->bone.num;i++)
		if (m->bone[i].model)
			unregister_model(m->bone[i].model);
}

void World::add_light(Light *l) {
	lights.add(l);
}

void World::add_particle(Particle *p) {
	particle_manager->add(p);
}

