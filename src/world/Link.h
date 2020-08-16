/*
 * Link.h
 *
 *  Created on: 16.08.2020
 *      Author: michi
 */

#ifndef SRC_WORLD_LINK_H_
#define SRC_WORLD_LINK_H_

#include "../y/Entity.h"

class vector;
class quaternion;
class Object;

class btTypedConstraint;



enum class LinkType {
	SOCKET,
	HINGE,
	UNIVERSAL,
	SPRING,
	SLIDER
};

class Link : public Entity {
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



#endif /* SRC_WORLD_LINK_H_ */
