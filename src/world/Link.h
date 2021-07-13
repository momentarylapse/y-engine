/*
 * Link.h
 *
 *  Created on: 16.08.2020
 *      Author: michi
 */

#pragma once

#include "../y/Entity.h"

class vector;
class quaternion;

class btTypedConstraint;

class Object;
class SolidBodyComponent;



enum class LinkType {
	SOCKET,
	HINGE,
	UNIVERSAL,
	SPRING,
	SLIDER
};

class Link : public Entity {
public:
	Link(LinkType type, Object *a, Object *b);
	~Link();

	void set_motor(float v, float max);
	void set_frame(int n, const quaternion &q);

	btTypedConstraint *con;
	LinkType link_type;
	SolidBodyComponent *a;
	SolidBodyComponent *b;

	void _create_link_data(vector &pa, vector &pb, quaternion &iqa, quaternion &iqb, const vector &pos);

	static Link* create(LinkType type, Object *a, Object *b, const vector &pos, const quaternion &ang);
};

class LinkSocket : public Link {
public:
	LinkSocket(Object *a, Object *b, const vector &pos);
	void __init__(Object *a, Object *b, const vector &pos);
};

class LinkHinge : public Link {
public:
	LinkHinge(Object *a, Object *b, const vector &pos, const quaternion &ang);
	void __init__(Object *a, Object *b, const vector &pos, const quaternion &ang);
};

class LinkUniversal : public Link {
public:
	LinkUniversal(Object *a, Object *b, const vector &pos, const quaternion &ang);
	void __init__(Object *a, Object *b, const vector &pos, const quaternion &ang);
};


