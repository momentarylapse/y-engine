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
#include "../fx/ParticleManager.h"
#endif

#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>
#include <BulletCollision/CollisionShapes/btConvexPointCloudShape.h>


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



quaternion bt_get_q(const btQuaternion &q) {
	quaternion r;
	r.x = q.x();
	r.y = q.y();
	r.z = q.z();
	r.w = q.w();
	return r;
}

vector bt_get_v(const btVector3 &v) {
	vector r;
	r.x = v.x();
	r.y = v.y();
	r.z = v.z();
	return r;
}

btVector3 bt_set_v(const vector &v) {
	return btVector3(v.x, v.y, v.z);
}

btQuaternion bt_set_q(const quaternion &q) {
	return btQuaternion(q.x, q.y, q.z, q.w);
}

btTransform bt_set_trafo(const vector &p, const quaternion &q) {
	btTransform trafo;
	trafo.setIdentity();
	trafo.setOrigin(bt_set_v(p));
	trafo.setRotation(bt_set_q(q));
	return trafo;
}

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







void GodInit() {
	Image im;
	tex_white = new nix::Texture();
	tex_black = new nix::Texture();
	im.create(16, 16, White);
	tex_white->overwrite(im);
	im.create(16, 16, Black);
	tex_black->overwrite(im);
}

void GodEnd() {
}


void myTickCallback(btDynamicsWorld *world, btScalar timeStep) {
	auto dispatcher = world->getDispatcher();
	int n = dispatcher->getNumManifolds();
	for (int i=0; i<n; i++) {
		auto contactManifold = dispatcher->getManifoldByIndexInternal(i);
		auto obA = const_cast<btCollisionObject*>(contactManifold->getBody0());
		auto obB = const_cast<btCollisionObject*>(contactManifold->getBody1());
		auto a = static_cast<Object*>(obA->getUserPointer());
		auto b = static_cast<Object*>(obB->getUserPointer());
		int np = contactManifold->getNumContacts();
		for (int j=0; j<np; j++) {
			auto &pt = contactManifold->getContactPoint(j);
			if (pt.getDistance() <= 0) {
				if (a->physics_data.active)
					a->on_collide_m(b, bt_get_v(pt.m_positionWorldOnB), bt_get_v(pt.m_normalWorldOnB));
				if (b->physics_data.active)
					b->on_collide_m(a, bt_get_v(pt.m_positionWorldOnA), -bt_get_v(pt.m_normalWorldOnB));
			}
		}
	}
}

World::World() {
//	ubo_light = nullptr;
//	ubo_fog = nullptr;

	particle_manager = new ParticleManager();


	physics_mode = PhysicsMode::FULL_EXTERNAL;
	collisionConfiguration = new btDefaultCollisionConfiguration();
	dispatcher = new btCollisionDispatcher(collisionConfiguration);
	overlappingPairCache = new btDbvtBroadphase();
	solver = new btSequentialImpulseConstraintSolver;
	dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, overlappingPairCache, solver, collisionConfiguration);

	dynamicsWorld->setInternalTickCallback(myTickCallback);


	terrain_object = new Object();
	terrain_object->update_matrix();

	reset();
}

World::~World() {
	delete dynamicsWorld;
	delete solver;
	delete overlappingPairCache;
	delete dispatcher;
	delete collisionConfiguration;
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

	physics_mode = PhysicsMode::FULL_EXTERNAL;
	engine.physics_enabled = false;
	engine.collisions_enabled = true;
	physics_num_steps = 10;
	physics_num_link_steps = 5;


	// physics
#ifdef _X_ALLOW_X_
	//LinksReset();
#endif
}

LevelData::LevelData() {
	ego_index = -1;
	background_color = Gray;

	gravity = v_0;
	fog.enabled = false;
	physics_enabled = false;
	physics_mode = PhysicsMode::FULL_EXTERNAL;
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
	world.physics_mode = ld.physics_mode;
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
	for (int i=0; i<skybox.num; i++) {
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
		if (!o.filename.is_empty()){
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

	for (auto &l: ld.links) {
		Object *b = nullptr;
		if (l.object[1] >= 0)
			b = objects[l.object[1]];
		add_link(l.type, objects[l.object[0]], b, l.pos, quaternion::rotation(l.ang));
	}

	scripts = ld.scripts;

	net_msg_enabled = true;
	return ok;
}

Link *World::add_link(LinkType type, Object *a, Object *b, const vector &pos, const quaternion &ang) {
	auto l = new Link(type, a, b, pos, ang);
	links.add(l);
	dynamicsWorld->addConstraint(l->con, true);
	return l;
}

Link::Link(LinkType t, Object *_a, Object *_b, const vector &pos, const quaternion &ang) {
	type = t;
	a = _a;
	b = _b;
	con = nullptr;
	auto iqa = a->ang.bar();
	auto iqb = quaternion::ID;
	vector pa = iqa * (pos - a->pos);
	vector pb = pos;
	if (b) {
		iqb = b->ang.bar();
		pb = iqb * (pos - b->pos);
	}
	if (type == LinkType::SOCKET) {
		if (b) {
			msg_write("-----------add socket 2");
			con = new btPoint2PointConstraint(
				*a->body,
				*b->body,
				bt_set_v(pa),
				bt_set_v(pb));
		} else {
			msg_write("-----------add socket 1");
			con = new btPoint2PointConstraint(
				*a->body,
				bt_set_v(pa));
		}
	} else if (type == LinkType::HINGE) {
		if (b) {
			msg_write("-----------add hinge 2");
			con = new btHingeConstraint(
				*a->body,
				*b->body,
				bt_set_v(pa),
				bt_set_v(pb),
				bt_set_v(iqa * ang * vector::EZ),
				bt_set_v(iqb * ang * vector::EZ),
				true);
		} else {
			msg_write("-----------add hinge 1");
			con = new btHingeConstraint(
				*a->body,
				bt_set_v(pa),
				bt_set_v(iqa * ang * vector::EZ),
				true);
		}
	} else if (type == LinkType::UNIVERSAL) {
		msg_write("-----------add universal");
		con = new btUniversalConstraint(
			*a->body,
			*b->body,
			bt_set_v(pos),
			bt_set_v(ang * vector::EZ),
			bt_set_v(ang * vector::EY));
		((btUniversalConstraint*)con)->setLimit(4, 0,0.1f);
	} else {
		throw Exception("unknown link: " + i2s((int)type));
	}
}

Link::~Link() {
}

void Link::set_motor(float v, float max) {
	if (type == LinkType::HINGE)
		((btHingeConstraint*)con)->enableAngularMotor(max > 0, v, max);
}

/*void Link::set_axis(const vector &v) {
	auto vv = bt_set_v(v);
	if (type == LinkType::HINGE)
		((btHingeConstraint*)con)->setAxis(vv);
	btTransform f = bt_set_trafo(v_0, quaternion::ID);
	((btHingeConstraint*)con)->setFrames(f,f);
}*/

void Link::set_frame(int n, const quaternion &q) {
	if (type == LinkType::HINGE) {
		if (n == 1)
			((btHingeConstraint*)con)->getBFrame().setRotation(bt_set_q(q));
		else
			((btHingeConstraint*)con)->getAFrame().setRotation(bt_set_q(q));
	}
}

static Array<float> hh;

Terrain *World::create_terrain(const Path &filename, const vector &pos) {
	Terrain *tt = new Terrain(filename, pos);

	float a=10000, b=0;
	for (float f: tt->height){
		a = min(a, f);
		b = max(b, f);
	}
	printf("%f   %f\n", a, b);

	msg_write(tt->pattern.str());

	//tt->colShape = new btStaticPlaneShape(btVector3(0,1,0), 0);
	hh.clear();
	for (int z=0; z<tt->num_z+1; z++)
		for (int x=0; x<tt->num_x+1; x++)
			hh.add(tt->height[x * (tt->num_z+1) + z]);
	auto hf = new btHeightfieldTerrainShape(tt->num_x+1, tt->num_z+1, hh.data, 1.0f, -600, 600, 1, PHY_FLOAT, false);
	hf->setLocalScaling(bt_set_v(tt->pattern + vector(0,1,0)));
	tt->colShape = hf;
	btTransform startTransform = bt_set_trafo(pos + vector(tt->pattern.x * tt->num_x, 0, tt->pattern.z * tt->num_z)/2, quaternion::ID);
	btScalar mass(0.f);
	btVector3 localInertia(0, 0, 0);

	//using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
	btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
	btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, tt->colShape, localInertia);
	tt->body = new btRigidBody(rbInfo);
	tt->body->setUserPointer(terrain_object);

	dynamicsWorld->addRigidBody(tt->body);

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

bool LevelData::load(const Path &filename) {
	world_filename = filename;

	xml::Parser p;
	p.load(engine.map_dir << filename.with(".world"));
	auto *meta = p.elements[0].find("meta");
	if (meta) {
		for (auto &e: meta->elements) {
			if (e.tag == "background") {
				background_color = s2c(e.value("color"));
				for (auto &ee: e.elements) {
					if (ee.tag == "skybox") {
						skybox_filename.add(ee.value("file"));
						skybox_ang.add(v_0);
					}
				}
			} else if (e.tag == "physics") {
				physics_enabled = e.value("enabled")._bool();
				if (e.value("mode") == "simple")
					physics_mode = PhysicsMode::SIMPLE;
				else if (e.value("mode") == "full")
					physics_mode = PhysicsMode::FULL_EXTERNAL;
				gravity = s2v(e.value("gravity"));
			} else if (e.tag == "fog") {
				fog.enabled = e.value("enabled")._bool();
				fog.mode = e.value("mode")._int();
				fog.start = e.value("start")._float();
				fog.end = e.value("end")._float();
				fog.distance = 1.0f / e.value("density")._float();
				fog._color = s2c(e.value("color"));
			} else if (e.tag == "script") {
				Script s;
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
				Camera c;
				c.pos = s2v(e.value("pos"));
				c.ang = s2v(e.value("ang"));
				c.fov = e.value("fov", f2s(pi/4, 3))._float();
				c.min_depth = e.value("minDepth", "1")._float();
				c.max_depth = e.value("maxDepth", "10000")._float();
				c.exposure = e.value("exposure", "1")._float();
				cameras.add(c);
			} else if (e.tag == "light") {
				Light l;
				l.radius = e.value("radius")._float();
				l.harshness = e.value("harshness")._float();
				l._color = s2c(e.value("color"));
				l.ang = s2v(e.value("ang"));
				if (e.value("type") == "directional")
					l.radius = -1;
				l.enabled = e.value("enabled", "true")._bool();
				lights.add(l);
			} else if (e.tag == "terrain") {
				Terrain t;
				t.filename = e.value("file");
				t.pos = s2v(e.value("pos"));
				terrains.add(t);
			} else if (e.tag == "object") {
				Object o;
				o.filename = e.value("file");
				o.name = e.value("name");
				o.pos = s2v(e.value("pos"));
				o.ang = s2v(e.value("ang"));
				o.vel = v_0;
				o.rot = v_0;
				if (e.value("role") == "ego")
					ego_index = objects.num;
				objects.add(o);
			} else if (e.tag == "link") {
				Link l;
				l.pos = s2v(e.value("pos"));
				l.ang = s2v(e.value("ang"));
				l.object[0] = e.value("a")._int();
				l.object[1] = e.value("b")._int();
				l.type = LinkType::SOCKET;
				if (e.value("type") == "hinge")
					l.type = LinkType::HINGE;
				if (e.value("type") == "universal")
					l.type = LinkType::UNIVERSAL;
				if (e.value("type") == "spring")
					l.type = LinkType::SPRING;
				links.add(l);
			}
		}
	}

	return true;
}

bool GodLoadWorld(const Path &filename) {
	LevelData level_data;
	bool ok = level_data.load(filename);
	ok &= world.load(level_data);
	return ok;
}


Object *World::create_object(const Path &filename, const string &name, const vector &pos, const quaternion &ang, int w_index) {
	if (engine.resetting_game)
		throw Exception("CreateObject during game reset");

	if (filename.is_empty())
		throw Exception("CreateObject: empty filename");

	//msg_write(on);
	auto *o = static_cast<Object*>(ModelManager::load(filename));

	o->script_data.name = name;
	o->pos = pos;
	o->ang = ang;
	o->update_matrix();
	o->update_theta();


	register_object(o, w_index);

	o->on_init();

	AddNetMsg(NET_MSG_CREATE_OBJECT, o->object_id, filename.str());

	return o;
}

void World::register_object(Object *o, int index) {
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

	if (o->phys->balls.num + o->phys->cylinders.num + o->phys->poly.num > 0) {
		auto comp = new btCompoundShape(false, 0);
		for (auto &b: o->phys->balls) {
			vector a = o->phys->vertex[b.index];
			auto bb = new btSphereShape(btScalar(b.radius));
			comp->addChildShape(bt_set_trafo(a, quaternion::ID), bb);
		}
		for (auto &c: o->phys->cylinders) {
			vector a = o->phys->vertex[c.index[0]];
			vector b = o->phys->vertex[c.index[1]];
			auto cc = new btCylinderShapeZ(bt_set_v(vector(c.radius, c.radius, (b - a).length() / 2)));
			auto q = quaternion::rotation((a-b).dir2ang());
			comp->addChildShape(bt_set_trafo((a+b)/2, q), cc);
			if (c.round) {
				auto bb1 = new btSphereShape(btScalar(c.radius));
				comp->addChildShape(bt_set_trafo(a, quaternion::ID), bb1);
				auto bb2 = new btSphereShape(btScalar(c.radius));
				comp->addChildShape(bt_set_trafo(b, quaternion::ID), bb2);
			}
		}
		for (auto &p: o->phys->poly) {
			if (false){
				Array<btVector3> v;
				Set<int> vv;
				for (int i=0; i<p.num_faces; i++)
					for (int k=0; k<p.face[i].num_vertices; k++){
						vv.add(p.face[i].index[k]);
					}
				for (int i: vv) {
					v.add(bt_set_v(o->phys->vertex[i]));
					msg_write(o->phys->vertex[i].str());
				}
				msg_write(v.num);
				auto pp = new btConvexPointCloudShape(&v[0], v.num, btVector3(1,1,1));
				msg_write(pp->getNumEdges());
				msg_write(pp->getNumPlanes());
				comp->addChildShape(bt_set_trafo(v_0, quaternion::ID), pp);
			} else {
				// ARGH, btConvexPointCloudShape not working
				//   let's use a crude box for now... (-_-)'
				vector a, b;
				a = b = o->phys->vertex[p.face[0].index[0]];
				for (int i=0; i<p.num_faces; i++)
					for (int k=0; k<p.face[i].num_vertices; k++){
						auto vv = o->phys->vertex[p.face[i].index[k]];
						a._min(vv);
						b._max(vv);
					}
				auto pp = new btBoxShape(bt_set_v((b-a) / 2));
				comp->addChildShape(bt_set_trafo((a+b)/2, quaternion::ID), pp);

			}
		}
		o->colShape = comp;
	}

	/*if (o->phys->balls.num > 0) {
		auto &b = o->phys->balls[0];
		o->colShape = new btSphereShape(btScalar(b.radius));
	} else if (o->phys->cylinders.num > 0) {
		auto &c = o->phys->cylinders[0];
		vector a = o->mesh[0]->vertex[c.index[0]];
		vector b = o->mesh[0]->vertex[c.index[1]];
		o->colShape = new btCylinderShapeZ(bt_set_v(vector(c.radius, c.radius, (b - a).length())));
	} else if (o->phys->poly.num > 0) {

	} else {
	}*/

	btTransform startTransform = bt_set_trafo(o->pos, o->ang);

	btScalar mass(o->physics_data.active ? o->physics_data.mass : 0);
	btVector3 localInertia(0, 0, 0);
	//if (isDynamic)
	if (o->colShape) {
		o->colShape->calculateLocalInertia(mass, localInertia);
		o->physics_data.theta_0._00 = localInertia.x();
		o->physics_data.theta_0._11 = localInertia.y();
		o->physics_data.theta_0._22 = localInertia.z();
	}

	//using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
	btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
	btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, o->colShape, localInertia);
	o->body = new btRigidBody(rbInfo);

	o->body->setUserPointer(o);
	o->update_mass();

	if (o->physics_data.active or o->physics_data.passive)
		dynamicsWorld->addRigidBody(o->body);

}



// un-object a model
void World::unregister_object(Object *m) {
	if (m->object_id < 0)
		return;


	if (m->body) {
		delete m->body->getMotionState();
		delete m->body;
		delete m->colShape;
		m->body = nullptr;
		m->colShape = nullptr;
	}

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

void World::iterate(float dt) {
	if (!engine.physics_enabled)
		return;

	if (physics_mode == PhysicsMode::FULL_EXTERNAL) {
		dynamicsWorld->setGravity(bt_set_v(gravity));
		dynamicsWorld->stepSimulation(dt, 10);

		btTransform trans;
		for (auto *o: objects)
			if (o->physics_data.active) {
				o->body->getMotionState()->getWorldTransform(trans);
				o->pos = bt_get_v(trans.getOrigin());
				o->ang = bt_get_q(trans.getRotation());
				o->vel = bt_get_v(o->body->getLinearVelocity());
				o->rot = bt_get_v(o->body->getAngularVelocity());
			}

	} else if (physics_mode == PhysicsMode::SIMPLE) {
		for (auto *o: objects)
			o->do_physics(dt);
	}
}

void World::add_light(Light *l) {
	lights.add(l);
}

void World::add_particle(Particle *p) {
	particle_manager->add(p);
}


void World::shift_all(const vector &dpos) {
	for (auto *t: terrains)
		t->pos += dpos;
	for (auto *o: objects)
		o->pos += dpos;
	particle_manager->shift_all(dpos);
}

vector World::get_g(const vector &pos) const {
	return gravity;
}

