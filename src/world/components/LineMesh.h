/*
 * LineMesh.h
 *
 *  Created on: Nov 12, 2022
 *      Author: michi
 */

#ifndef WORLD_COMPONENTS_LINEMESH_H
#define WORLD_COMPONENTS_LINEMESH_H

#include "PointMesh.h"

struct LineVertex {
	vec3 pos;
	float r;
	color col;
};

class LineMesh : public TypedMesh<LineVertex> {
public:
	LineMesh();

	bool contiguous = false;
	bool width_in_screen_space = false;

	static const kaba::Class *_class;
};


#endif //WORLD_COMPONENTS_LINEMESH_H
