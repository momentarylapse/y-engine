/*
 * PointMesh.h
 *
 *  Created on: Nov 12, 2022
 *      Author: michi
 */

#ifndef WORLD_COMPONENTS_POINTMESH_H
#define WORLD_COMPONENTS_POINTMESH_H

#include "../../y/Component.h"
#include "../../lib/base/base.h"
#include "../../lib/base/pointer.h"
#include "../../lib/math/vec3.h"
#include "../../lib/image/color.h"
#include "../../graphics-impl.h"


class Material;

template<typename V>
class TypedMesh : public Component {
public:
	TypedMesh(const string &fmt) {
		vertex_buffer = new VertexBuffer(fmt);
	}

	using Vertex = V;

	Array<Vertex> vertices;

	Material *material = nullptr;

	VertexBuffer *vertex_buffer = nullptr;

	void update() {
		vertex_buffer->update(vertices);
	}
};


struct PointVertex {
	vec3 pos;
	float r;
	color col;
};

class Material;

class PointMesh : public TypedMesh<PointVertex> {
public:
	PointMesh();

	bool width_in_screen_space = false;
	
	static const kaba::Class *_class;
};


#endif //WORLD_COMPONENTS_POINTMESH_H
