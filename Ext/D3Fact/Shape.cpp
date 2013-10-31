/*
	This file is part of the MinSG library extension D3Fact.
	Copyright (C) 2010-2013 Sascha Brandt <myeti@mail.upb.de>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_D3FACT

#include "Shape.h"

#include <Geometry/Box.h>
#include <Geometry/Matrix4x4.h>

#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Mesh/VertexDescription.h>
#include <Rendering/Mesh/MeshVertexData.h>
#include <Rendering/MeshUtils/PlatonicSolids.h>
#include <Rendering/MeshUtils/MeshUtils.h>
#include <Rendering/MeshUtils/MeshBuilder.h>

#include <Util/StringIdentifier.h>
#include <Util/Graphics/ColorLibrary.h>

using namespace Rendering;

namespace D3Fact {


Mesh* Shape::createShape(const std::string& shape, const std::vector<Geometry::Vec3>& points) {
	static const Util::StringIdentifier SHAPE_BOX("box");
	static const Util::StringIdentifier SHAPE_QUAD("quad");
	static const Util::StringIdentifier SHAPE_DISC("disc");
	static const Util::StringIdentifier SHAPE_PATH("path");
	static const Util::StringIdentifier SHAPE_SPHERE("sphere");
	static const Util::StringIdentifier SHAPE_OPEN_CYLINDER("open_cylinder");
	static const Util::StringIdentifier SHAPE_CLOSED_CYLINDER("closed_cylinder");
	static const Util::StringIdentifier SHAPE_TORUS("torus");
	static const Util::StringIdentifier SHAPE_GRID("grid");
	const Util::StringIdentifier shapeId(shape);
	Mesh* mesh = nullptr;
	if(shapeId == SHAPE_BOX) {
		VertexDescription vertexDescription;
		vertexDescription.appendPosition3D();
		vertexDescription.appendNormalFloat();
		Geometry::Box box(points.front(), 0);
		for(const auto& p : points) {
			box.include(p);
		}
		mesh = MeshUtils::MeshBuilder::createBox(vertexDescription, box);
	} else if(shapeId == SHAPE_QUAD) {
		VertexDescription vertexDescription;
		vertexDescription.appendPosition3D();
		vertexDescription.appendNormalFloat();
		vertexDescription.appendTexCoord();
		MeshUtils::MeshBuilder b(vertexDescription);

		b.normal( Geometry::Vec3(0,0,1.0f) );
		b.color(Util::ColorLibrary::WHITE);

		b.texCoord0( Geometry::Vec2(0,0) );
		b.position( points.at(0) );
		b.addVertex();

		b.texCoord0( Geometry::Vec2(0,1) );
		b.position( points.at(1) );
		b.addVertex();

		b.texCoord0( Geometry::Vec2(1,1) );
		b.position(points.at(2) );
		b.addVertex();

		b.texCoord0( Geometry::Vec2(1,0) );
		b.position( points.at(3) );
		b.addVertex();

		b.addQuad(3,2,1,0);
		mesh = b.buildMesh();
	} else if(shapeId == SHAPE_PATH) {
		VertexDescription vertexDescription;
		vertexDescription.appendPosition3D();
		MeshUtils::MeshBuilder b(vertexDescription);
		for(const auto& p : points) {
			b.position( p );
			b.addIndex(b.addVertex());
		}
		mesh = b.buildMesh();
		mesh->setDrawMode(Mesh::DRAW_LINE_STRIP);
	} else if(shapeId == SHAPE_DISC) {
		const Geometry::Vec3 center = points.at(0);
		const Geometry::Vec3 scale = points.at(1);
		mesh = MeshUtils::MeshBuilder::createDiscSector(0.5, 32 );
		MeshVertexData& md = mesh->openVertexData();
		Geometry::Matrix4x4f mat;
		mat.rotate_deg(90, 0, 1, 0);
		mat.scale(scale.x(), scale.y(), 1);
		mat.translate(center);
		MeshUtils::transform(md, mat);
		md.markAsChanged();
	} else if(shapeId == SHAPE_SPHERE) {
		const Geometry::Vec3 center = points.at(0);
		const Geometry::Vec3 scale = points.at(1);
		VertexDescription vertexDescription;
		vertexDescription.appendPosition3D();
		vertexDescription.appendNormalFloat();
		vertexDescription.appendTexCoord();
		mesh = MeshUtils::MeshBuilder::createSphere(vertexDescription, 16, 16);
		MeshVertexData& md = mesh->openVertexData();
		Geometry::Matrix4x4f mat;
		mat.rotate_deg(90, 1, 0, 0);
		mat.scale(scale.x(), scale.y(), scale.z());
		mat.translate(center);
		MeshUtils::transform(md, mat);
		md.markAsChanged();
	}
	return mesh;
}

void Shape::updateShape(const std::string& shape, Rendering::Mesh* mesh, const std::vector<Geometry::Vec3>& points) {
	Mesh* m = createShape(shape, points);
	mesh->swap(*m);
	delete m;
}

} /* namespace D3Fact */


#endif /* MINSG_EXT_D3FACT */
