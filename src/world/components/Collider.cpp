/*
 * Collider.cpp
 *
 *  Created on: Jul 14, 2021
 *      Author: michi
 */

#include "Collider.h"
#include "../Model.h"
#include "../Terrain.h"
#include "../../lib/base/set.h"
#include "../../lib/file/msg.h"

#if HAS_LIB_BULLET
#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>


btVector3 bt_set_v(const vector &v);
btQuaternion bt_set_q(const quaternion &q);
vector bt_get_v(const btVector3 &v);
btTransform bt_set_trafo(const vector &p, const quaternion &q);
#endif


const kaba::Class *Collider::_class = nullptr;
const kaba::Class *MeshCollider::_class = nullptr;
const kaba::Class *SphereCollider::_class = nullptr;
const kaba::Class *BoxCollider::_class = nullptr;
const kaba::Class *TerrainCollider::_class = nullptr;

Collider::Collider() {
	msg_write("COLLIDER");
	col_shape = nullptr;
}

Collider::~Collider() {
	if (col_shape)
		delete col_shape;
}


MeshCollider::MeshCollider() {}

void MeshCollider::on_init() {
	auto o = get_owner<Model>();
#if HAS_LIB_BULLET
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
			if (true){
				Set<int> vv;
				for (int i=0; i<p.num_faces; i++)
					for (int k=0; k<p.face[i].num_vertices; k++){
						vv.add(p.face[i].index[k]);
					}
				// btConvexPointCloudShape not working!
				auto pp = new btConvexHullShape();
				for (int i: vv)
					pp->addPoint(bt_set_v(o->phys->vertex[i]));
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
		col_shape = comp;
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
#endif
}


TerrainCollider::TerrainCollider(){}

void TerrainCollider::on_init() {
	auto t = (Terrain*)owner->get_component(Terrain::_class);

	float a = 0, b = 0;
	for (float f: t->height){
		a = min(a, f);
		b = max(b, f);
	}
	float d = max(abs(a), abs(b)) + 10;

	//tt->colShape = new btStaticPlaneShape(btVector3(0,1,0), 0);

	// transpose the array for bullet
	for (int z=0; z<t->num_z+1; z++)
		for (int x=0; x<t->num_x+1; x++)
			hh.add(t->height[x * (t->num_z+1) + z]);
	// data is only referenced by bullet!  (keep)

#if HAS_LIB_BULLET
	auto hf = new btHeightfieldTerrainShape(t->num_x+1, t->num_z+1, hh.data, 1.0f, -d, d, 1, PHY_FLOAT, false);
	hf->setLocalScaling(bt_set_v(t->pattern + vector(0,1,0)));

	// bullet assumes the origin in the center of the terrain!
	auto comp = new btCompoundShape(false, 0);
	comp->addChildShape(bt_set_trafo(vector(t->pattern.x * t->num_x, 0, t->pattern.z * t->num_z)/2, quaternion::ID), hf);
	col_shape = comp;
#endif
}
