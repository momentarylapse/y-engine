/*
 * PointMesh.h
 *
 *  Created on: Nov 12, 2022
 *      Author: michi
 */

#include "PointMesh.h"
#include "../../graphics-impl.h"

const kaba::Class *PointMesh::_class = nullptr;


PointMesh::PointMesh() : SimpleMesh<PointVertex>("4f,f,4f") {
}

