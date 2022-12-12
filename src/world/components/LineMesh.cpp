/*
 * LineMesh.h
 *
 *  Created on: Nov 12, 2022
 *      Author: michi
 */

#include "LineMesh.h"
#include "../../graphics-impl.h"

const kaba::Class *LineMesh::_class = nullptr;


LineMesh::LineMesh() : TypedMesh<LineVertex>("3f,f,4f") {
}

