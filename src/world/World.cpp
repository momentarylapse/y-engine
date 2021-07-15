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
#include "../lib/file/file.h"
//#include "../lib/vulkan/vulkan.h"
#include "../lib/nix/nix.h"
#include "../y/EngineData.h"
#include "../y/Component.h"
#include "../y/ComponentManager.h"
#include "../meta.h"
#include "ModelManager.h"
#include "Link.h"
#include "Material.h"
#include "Model.h"
#include "Object.h"
#include "Terrain.h"
#include "World.h"

#include "components/SolidBody.h"
#include "components/Collider.h"
#include "components/Animator.h"

#ifdef _X_ALLOW_X_
#include "Light.h"
#include "../fx/Particle.h"
#include "../fx/ParticleManager.h"
#include "../plugins/PluginManager.h"
#endif

#ifdef _X_ALLOW_X_
#include "../audio/Sound.h"
#endif

#if HAS_LIB_BULLET
#include <btBulletDynamicsCommon.h>
//#include <BulletCollision/CollisionShapes/btConvexPointCloudShape.h>
#include <BulletCollision/CollisionShapes/btConvexHullShape.h>
#endif

#include "Camera.h"


//vulkan::DescriptorSet *rp_create_dset(const Array<vulkan::Texture*> &tex, vulkan::UniformBuffer *ubo);



//#define _debug_matrices_


#if HAS_LIB_BULLET
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
#endif

// game data
World world;


#ifdef _X_ALLOW_X_
void DrawSplashScreen(const string &str, float per);
#else
void DrawSplashScreen(const string &str, float per){}
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

inline bool TestVectorSanity(vector &v, const char *name) {
	if (inf_v(v)) {
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
}

void GodEnd() {
}

void send_collision(Model *m, const CollisionData &col) {
	for (auto c: m->components)
		c->on_collide(col);
}

#if HAS_LIB_BULLET
void myTickCallback(btDynamicsWorld *world, btScalar timeStep) {
	auto dispatcher = world->getDispatcher();
	int n = dispatcher->getNumManifolds();
	//CollisionData col;
	for (int i=0; i<n; i++) {
		auto contactManifold = dispatcher->getManifoldByIndexInternal(i);
		auto obA = const_cast<btCollisionObject*>(contactManifold->getBody0());
		auto obB = const_cast<btCollisionObject*>(contactManifold->getBody1());
		auto a = static_cast<SolidBody*>(obA->getUserPointer());
		auto b = static_cast<SolidBody*>(obB->getUserPointer());
		int np = contactManifold->getNumContacts();
		for (int j=0; j<np; j++) {
			auto &pt = contactManifold->getContactPoint(j);
			if (pt.getDistance() <= 0) {
				if (a->active)
					send_collision((Model*)a->owner, {(Model*)b->owner, nullptr, nullptr, bt_get_v(pt.m_positionWorldOnB), bt_get_v(pt.m_normalWorldOnB)});
				if (b->active)
					send_collision((Model*)b->owner, {(Model*)a->owner, nullptr, nullptr, bt_get_v(pt.m_positionWorldOnA), -bt_get_v(pt.m_normalWorldOnB)});
			}
		}
	}
}
#endif

World::World() {
//	ubo_light = nullptr;
//	ubo_fog = nullptr;

#ifdef _X_ALLOW_X_
	particle_manager = new ParticleManager();
#endif


	physics_mode = PhysicsMode::FULL_EXTERNAL;
#if HAS_LIB_BULLET
	collisionConfiguration = new btDefaultCollisionConfiguration();
	dispatcher = new btCollisionDispatcher(collisionConfiguration);
	overlappingPairCache = new btDbvtBroadphase();
	solver = new btSequentialImpulseConstraintSolver;
	dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, overlappingPairCache, solver, collisionConfiguration);
	dynamicsWorld->setInternalTickCallback(myTickCallback);
#endif


	reset();
}

World::~World() {
#if HAS_LIB_BULLET
	delete dynamicsWorld;
	delete solver;
	delete overlappingPairCache;
	delete dispatcher;
	delete collisionConfiguration;
#endif
}

void World::reset() {
	net_msg_enabled = false;
	net_messages.clear();

	gravity = v_0;

	// terrains
	//for (auto *t: terrains)
	//	delete t;
	for (auto *o: terrain_objects)
		delete o;
	terrain_objects.clear();
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

#ifdef _X_ALLOW_X_
	for (auto *l: lights)
		delete l;
	lights.clear();

	particle_manager->clear();
#endif


#ifdef _X_ALLOW_X_
	// music
	for (auto *s: sounds)
		delete s;
	sounds.clear();
	/*if (meta->MusicEnabled){
		NixSoundStop(MusicCurrent);
	}*/
#endif

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
		auto *ll = new Light(l.pos, quaternion::rotation(l.ang), l._color, l.radius, l.theta);
		ll->light.harshness = l.harshness;
		ll->enabled = l.enabled;
		if (ll->light.radius < 0)
			ll->allow_shadow = true;
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
			Object *oo = create_object_x(o.filename, o.name, o.pos, q, o.components, i);
			ok &= (oo != nullptr);
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
		add_link(Link::create(l.type, objects[l.object[0]], b, l.pos, quaternion::rotation(l.ang)));
	}

	scripts = ld.scripts;

	net_msg_enabled = true;
	return ok;
}

void World::add_link(Link *l) {
	links.add(l);
#if HAS_LIB_BULLET
	dynamicsWorld->addConstraint(l->con, true);
#endif
}


Terrain *World::create_terrain(const Path &filename, const vector &pos) {
	msg_error("TERRAIN");

	auto o = new Object();
	o->update_matrix();
	o->pos = pos;
	terrain_objects.add(o);

	auto t = new Terrain(filename);
	t->type = Terrain::_class;
	o->_add_component_external_(t);

	auto col = new TerrainCollider(t);
	col->type = TerrainCollider::_class;
	o->_add_component_external_(col);


	auto sb = new SolidBody(o);
	sb->type = SolidBody::_class;
	o->_add_component_external_(sb);

	//auto sb = (SolidBodyComponent*)o->add_component(SolidBodyComponent::_class, "");
#if HAS_LIB_BULLET
	dynamicsWorld->addRigidBody(sb->body);
#endif

	terrains.add(t);
	return t;
}

bool GodLoadWorld(const Path &filename) {
	LevelData level_data;
	bool ok = level_data.load(filename);
	ok &= world.load(level_data);
	return ok;
}

Object *World::create_object(const Path &filename, const vector &pos, const quaternion &ang) {
	return create_object_x(filename, "", pos, ang, {});
}

Object *World::create_object_x(const Path &filename, const string &name, const vector &pos, const quaternion &ang, const Array<LevelData::ScriptData> &components, int w_index) {
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


	register_object(o, w_index);

	// for now...
	if (o->physics_data_.active or o->physics_data_.passive) {
		// TODO

		auto col = new MeshCollider(o);
		col->type = MeshCollider::_class;
		o->_add_component_external_(col);

		auto sb = new SolidBody(o);
		sb->type = SolidBody::_class;
		o->_add_component_external_(sb);

		//auto sb = (SolidBodyComponent*)o->add_component(SolidBodyComponent::_class, "");
		dynamicsWorld->addRigidBody(sb->body);
	}

	if (o->uses_bone_animations()) {
		auto ani = new Animator(o);
		ani->type = Animator::_class;
		o->_add_component_external_(ani);
	}


	for (auto &cc: components) {
		//msg_write("add component " + cc.class_name);
#ifdef _X_ALLOW_X_
		auto type = plugin_manager.find_class(cc.filename, cc.class_name);
		auto comp = o->add_component(type, cc.var);
#endif
	}

	o->on_init();

	AddNetMsg(NET_MSG_CREATE_OBJECT, o->object_id, filename.str());

	return o;
}

Object* World::create_object_multi(const Path &filename, const Array<vector> &pos, const Array<quaternion> &ang) {


	//msg_write(on);
	auto *o = static_cast<Object*>(ModelManager::load(filename));

	Array<matrix> matrices;
	for (int i=0; i<pos.num; i++) {
		matrices.add(matrix::translation(pos[i]) * matrix::rotation(ang[i]));
	}
	register_model_multi(o, matrices);

	return o;
}

void World::register_model_multi(Model *m, const Array<matrix> &matrices) {

	if (m->registered)
		return;

	for (int i=0;i<m->material.num;i++){
		Material *mat = m->material[i];

		PartialModelMulti p;
		p.model = m;
		p.material = mat;
		p.matrices = matrices;
		p.mat_index = i;
		p.transparent = false;
		p.shadow = false;
		sorted_multi.add(p);
	}

	m->registered = true;
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
}


void World::set_active_physics(Object *o, bool active, bool passive) { //, bool test_collisions) {
	auto b = reinterpret_cast<SolidBody*>(o->get_component(SolidBody::_class));
	auto c = reinterpret_cast<Collider*>(o->get_component(Collider::_class));

#if HAS_LIB_BULLET
	btScalar mass(active ? b->mass : 0);
	btVector3 localInertia(0, 0, 0);
	if (c->col_shape) {
		c->col_shape->calculateLocalInertia(mass, localInertia);
		b->theta_0._00 = localInertia.x();
		b->theta_0._11 = localInertia.y();
		b->theta_0._22 = localInertia.z();
	}
	b->body->setMassProps(mass, localInertia);
	if (passive and !b->passive)
		dynamicsWorld->addRigidBody(b->body);
	if (!passive and b->passive)
		dynamicsWorld->removeRigidBody(b->body);

	/*if (!passive and test_collisions) {
		msg_error("FIXME pure collision");
		dynamicsWorld->addCollisionObject(o->body);
	}*/
#endif


	b->active = active;
	b->passive = passive;
	//b->test_collisions = test_collisions;
}

// un-object a model
void World::unregister_object(Object *m) {
	if (m->object_id < 0)
		return;

#if HAS_LIB_BULLET
	auto *sb = (SolidBody*)m->get_component(SolidBody::_class);
	if (sb) {
		dynamicsWorld->removeRigidBody(sb->body);
	}
#endif

	// ego...
	if (m == ego)
		ego = NULL;

	AddNetMsg(NET_MSG_DELETE_OBJECT, m->object_id, "");

	// remove from list
	objects[m->object_id] = NULL;
	m->object_id = -1;
}

void World::_delete(Entity* x) {
	if (unregister(x)) {
		msg_error("DEL");
		x->on_delete_rec();
		delete x;
	}
}


bool World::unregister(Entity* x) {
	//msg_error("World.unregister  " + i2s((int)x->type));
	if (x->type == Entity::Type::MODEL) {
		foreachi(auto *o, objects, i)
			if (o == x) {
				//msg_write(" -> OBJECT");
				unregister_model(o);
				unregister_object(o);
				return true;
			}
	} else if (x->type == Entity::Type::LIGHT) {
#ifdef _X_ALLOW_X_
		foreachi(auto *l, lights, i)
			if (l == x) {
				//msg_write(" -> LIGHT");
				lights.erase(i);
				return true;
			}
#endif
	} else if (x->type == Entity::Type::LINK) {
		foreachi(auto *l, links, i)
			if (l == x) {
				//msg_write(" -> LINK");
				links.erase(i);
				return true;
			}
	} else if (x->type == Entity::Type::SOUND) {
#ifdef _X_ALLOW_X_
		foreachi(auto *s, sounds, i)
			if (s == x) {
				//msg_write(" -> SOUND");
				sounds.erase(i);
				return true;
			}
#endif
	} else if (x->type == Entity::Type::PARTICLE or x->type == Entity::Type::BEAM) {
#ifdef _X_ALLOW_X_
		if (particle_manager->unregister((Particle*)x))
			return true;
#endif
	}
	return false;
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
	msg_write("unreg model " + m->filename().str());

	foreachib (auto &s, sorted_trans, i)
		if (s.model == m) {
			s.clear();
			sorted_trans.erase(i);
		}
	foreachib (auto &s, sorted_opaque, i)
		if (s.model == m) {
			msg_write("R");
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

void World::iterate_physics(float dt) {
	auto list = ComponentManager::get_listx<SolidBody>();

	if (physics_mode == PhysicsMode::FULL_EXTERNAL) {
#if HAS_LIB_BULLET
		dynamicsWorld->setGravity(bt_set_v(gravity));
		dynamicsWorld->stepSimulation(dt, 10);

		for (auto *o: *list)
			o->get_state_from_bullet();
#endif
	} else if (physics_mode == PhysicsMode::SIMPLE) {
		for (auto *o: *list)
			o->do_simple_physics(dt);
	}

	for (auto *sb: *list) {
		auto o = sb->get_owner<Model>();
		o->update_matrix();
	}
}

void World::iterate_animations(float dt) {
	auto list = ComponentManager::get_listx<Animator>();
	for (auto *o: *list)
		o->do_animation(dt);
}

void World::iterate(float dt) {
	if (dt == 0)
		return;
	if (engine.physics_enabled) {
		iterate_physics(dt);
	} else {
		for (auto *o: objects)
			if (o)
				o->update_matrix();
	}

#ifdef _X_ALLOW_X_
	foreachi (auto *s, sounds, i) {
		if (s->suicidal and s->has_ended()) {
			sounds.erase(i);
			delete s;
		}
	}
	audio::set_listener(cam->pos, cam->ang, v_0, 100000);
#endif
}

void World::add_light(Light *l) {
	lights.add(l);
}

void World::add_particle(Particle *p) {
#ifdef _X_ALLOW_X_
	particle_manager->add(p);
#endif
}

void World::add_sound(audio::Sound *s) {
	sounds.add(s);
}


void World::shift_all(const vector &dpos) {
	for (auto *t: terrains)
		t->get_owner<Model>()->pos += dpos;
	for (auto *o: objects)
		if (o)
			o->pos += dpos;
#ifdef _X_ALLOW_X_
	for (auto *s: sounds)
		s->pos += dpos;
	particle_manager->shift_all(dpos);
#endif
}

vector World::get_g(const vector &pos) const {
	return gravity;
}

bool World::trace(const vector &p1, const vector &p2, CollisionData &d, bool simple_test, Model *o_ignore) {
#if HAS_LIB_BULLET
	btCollisionWorld::ClosestRayResultCallback ray_callback(bt_set_v(p1), bt_set_v(p2));
	//ray_callback.m_collisionFilterMask = FILTER_CAMERA;

// Perform raycast
	this->dynamicsWorld->getCollisionWorld()->rayTest(bt_set_v(p1), bt_set_v(p2), ray_callback);
	if (ray_callback.hasHit()) {
		auto sb = static_cast<SolidBody*>(ray_callback.m_collisionObject->getUserPointer());
		d.p = bt_get_v(ray_callback.m_hitPointWorld);
		d.n = bt_get_v(ray_callback.m_hitNormalWorld);
		d.m = sb->get_owner<Model>();

		// ignore...
		if (d.m and d.m == o_ignore) {
			vector dir = (p2 - p1).normalized();
			return trace(d.p + dir * 2, p2, d, simple_test, o_ignore);
		}
		return true;
	}
#endif
	return false;
}

