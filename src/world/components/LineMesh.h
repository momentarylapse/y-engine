/*
 * LineMesh.h
 *
 *  Created on: Nov 12, 2022
 *      Author: michi
 */

#ifndef WORLD_COMPONENTS_LINEMESH_H
#define WORLD_COMPONENTS_LINEMESH_H

#include "../../y/Component.h"
#include "../../lib/base/base.h"
#include "../../lib/base/pointer.h"
#include "../../lib/math/vec3.h"
#include "../../lib/image/color.h"
#include "../../graphics-fwd.h"

struct LineVertex {
	vec3 pos;
	float r;
	color col;
};

class Material;

class LineMesh : public Component {
public:
	LineMesh();

	bool contiguous = false;
	bool width_in_screen_space = false;

	Array<LineVertex> vertices;

	Material *material = nullptr;

	void update();

	VertexBuffer *vertex_buffer = nullptr;

	static const kaba::Class *_class;
};


#endif //WORLD_COMPONENTS_LINEMESH_H
