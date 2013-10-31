/*
	This file is part of the MinSG library extension Triangulation.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_TRIANGULATION

#include "Helper.h"
#include <Geometry/Tetrahedron.h>
#include <Rendering/Mesh/Mesh.h>
#include <Rendering/MeshUtils/MeshBuilder.h>

namespace MinSG {
namespace Triangulation {

Rendering::Mesh * createTriangle2DMesh(const Geometry::Vec2f & a, const Geometry::Vec2f & b, const Geometry::Vec2f & c) {
	Rendering::VertexDescription vertexDescription;
	vertexDescription.appendPosition2D();
	vertexDescription.appendNormalFloat();

	Rendering::MeshUtils::MeshBuilder mb(vertexDescription);
	mb.normal(Geometry::Vec3(0, 0, -1));
	mb.position(a);
	mb.addVertex();
	mb.position(b);
	mb.addVertex();
	mb.position(c);
	mb.addVertex();
	mb.addTriangle(0, 1, 2);

	return mb.buildMesh();
}

Rendering::Mesh * createTetrahedronMesh(const Geometry::Tetrahedron<float> & tetrahedron) {
	Rendering::VertexDescription vertexDescription;
	vertexDescription.appendPosition3D();
	vertexDescription.appendColorRGBAFloat();
	vertexDescription.appendNormalFloat();
	Rendering::MeshUtils::MeshBuilder mb(vertexDescription);

	uint32_t i;

	// pA cbd
	mb.normal(tetrahedron.getPlaneA().getNormal());
	mb.position(tetrahedron.getVertexC());	i=mb.addVertex();
	mb.position(tetrahedron.getVertexB());	mb.addVertex();
	mb.position(tetrahedron.getVertexD());	mb.addVertex();
	mb.addTriangle(i,i+1,i+2);

	// pB acd
	mb.normal(tetrahedron.getPlaneB().getNormal());
	mb.position(tetrahedron.getVertexA());	i=mb.addVertex();
	mb.position(tetrahedron.getVertexC());	mb.addVertex();
	mb.position(tetrahedron.getVertexD());	mb.addVertex();
	mb.addTriangle(i,i+1,i+2);

	// pC adb
	mb.normal(tetrahedron.getPlaneC().getNormal());
	mb.position(tetrahedron.getVertexA());	i=mb.addVertex();
	mb.position(tetrahedron.getVertexD());	mb.addVertex();
	mb.position(tetrahedron.getVertexB());	mb.addVertex();
	mb.addTriangle(i,i+1,i+2);

	// pD abc
	mb.normal(tetrahedron.getPlaneD().getNormal());
	mb.position(tetrahedron.getVertexA());	i=mb.addVertex();
	mb.position(tetrahedron.getVertexB());	mb.addVertex();
	mb.position(tetrahedron.getVertexC());	mb.addVertex();
	mb.addTriangle(i,i+1,i+2);

	return mb.buildMesh();
}

}
}

#endif /* MINSG_EXT_TRIANGULATION */
