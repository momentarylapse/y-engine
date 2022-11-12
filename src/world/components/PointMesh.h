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
#include "../../graphics-fwd.h"

struct PointVertex {
	vec3 pos;
	float r;
	color col;
};

class Material;

class PointMesh : public Component {
public:
	PointMesh();

	bool width_in_screen_space = false;

	Array<PointVertex> vertices;

	Material *material = nullptr;

	void update();

	VertexBuffer *vertex_buffer = nullptr;

	static const kaba::Class *_class;
};


#endif //WORLD_COMPONENTS_POINTMESH_H
