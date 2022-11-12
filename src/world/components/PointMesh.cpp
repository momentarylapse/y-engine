/*
 * PointMesh.h
 *
 *  Created on: Nov 12, 2022
 *      Author: michi
 */

#include "PointMesh.h"
#include "../../graphics-impl.h"

const kaba::Class *PointMesh::_class = nullptr;


PointMesh::PointMesh() {

}

void PointMesh::update() {
	if (!vertex_buffer)
		vertex_buffer = new VertexBuffer("3f,f,4f");
	vertex_buffer->update(vertices);
}
