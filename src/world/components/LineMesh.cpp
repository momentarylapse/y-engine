/*
 * LineMesh.h
 *
 *  Created on: Nov 12, 2022
 *      Author: michi
 */

#include "LineMesh.h"
#include "../../graphics-impl.h"

const kaba::Class *LineMesh::_class = nullptr;


LineMesh::LineMesh() {

}

void LineMesh::update() {
	if (!vertex_buffer)
		vertex_buffer = new VertexBuffer("3f,f,4f");
	vertex_buffer->update(vertices);
}
