/*
 * Collider.h
 *
 *  Created on: Jul 14, 2021
 *      Author: michi
 */

#pragma once

#include "../../y/Component.h"

class Model;
class Terrain;

class btCollisionShape;

class Collider : public Component {
public:
	Collider();
	virtual ~Collider();

	btCollisionShape* col_shape;

	static const kaba::Class *_class;
};

class MeshCollider : public Collider {
public:
	MeshCollider();
	void on_init() override;
	static const kaba::Class *_class;
};

class SphereCollider : public Collider {
public:
	SphereCollider();
	static const kaba::Class *_class;
};

class BoxCollider : public Collider {
public:
	BoxCollider();
	static const kaba::Class *_class;
};

class TerrainCollider : public Collider {
public:
	TerrainCollider();
	void on_init() override;
	Array<float> hh;
	static const kaba::Class *_class;
};

