/*
 * Link.cpp
 *
 *  Created on: 16.08.2020
 *      Author: michi
 */

#include "Link.h"
#include "object.h"
#include "../lib/file/msg.h"



#include <btBulletDynamicsCommon.h>

btVector3 bt_set_v(const vector &v);
btQuaternion bt_set_q(const quaternion &q);



Link::Link(LinkType t, Object *_a, Object *_b, const vector &pos, const quaternion &ang) : Entity(Type::LINK) {
	link_type = t;
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
	if (link_type == LinkType::SOCKET) {
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
	} else if (link_type == LinkType::HINGE) {
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
	} else if (link_type == LinkType::UNIVERSAL) {
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
	if (link_type == LinkType::HINGE)
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
	if (link_type == LinkType::HINGE) {
		if (n == 1)
			((btHingeConstraint*)con)->getBFrame().setRotation(bt_set_q(q));
		else
			((btHingeConstraint*)con)->getAFrame().setRotation(bt_set_q(q));
	}
}


