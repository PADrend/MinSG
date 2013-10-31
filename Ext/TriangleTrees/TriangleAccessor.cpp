/*
	This file is part of the MinSG library extension TriangleTrees.
	Copyright (C) 2009-2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_TRIANGLETREES

#include "TriangleAccessor.h"
#include <Geometry/Triangle.h>
#include <Geometry/Vec3.h>
#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Mesh/MeshIndexData.h>
#include <Rendering/Mesh/MeshVertexData.h>
#include <Rendering/Mesh/VertexAttribute.h>
#include <Rendering/Mesh/VertexAttributeIds.h>
#include <Rendering/Mesh/VertexDescription.h>

namespace MinSG {
namespace TriangleTrees {

TriangleAccessor::TriangleAccessor(const Rendering::Mesh * _mesh, uint32_t _triangleIndex) : mesh(_mesh), triangleIndex(_triangleIndex) {
	const Rendering::VertexAttribute & posAttr = mesh->getVertexDescription().getAttribute(Rendering::VertexAttributeIds::POSITION);
	const uint16_t offset = posAttr.getOffset();
	vertexPositions[0] = reinterpret_cast<const float *>(getVertexData(0) + offset);
	vertexPositions[1] = reinterpret_cast<const float *>(getVertexData(1) + offset);
	vertexPositions[2] = reinterpret_cast<const float *>(getVertexData(2) + offset);
}

const uint8_t * TriangleAccessor::getVertexData(unsigned char num) const {
	const Rendering::MeshIndexData & indexData = mesh->_getIndexData();
	const Rendering::MeshVertexData & vertexData = mesh->_getVertexData();

	const uint32_t & vertexIndex = indexData[3 * triangleIndex + num];
	return vertexData[vertexIndex];
}

Geometry::Triangle_f TriangleAccessor::getTriangle() const {
	return Geometry::Triangle_f(Geometry::Vec3f(getVertexPosition(0)),
								Geometry::Vec3f(getVertexPosition(1)),
								Geometry::Vec3f(getVertexPosition(2)));
}

}
}

#endif /* MINSG_EXT_TRIANGLETREES */
