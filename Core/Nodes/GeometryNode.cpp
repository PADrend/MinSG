/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "GeometryNode.h"

#include "GroupNode.h"
#include "../FrameContext.h"
#include "../Statistics.h"

#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Draw.h>
#include <Util/Graphics/ColorLibrary.h>

namespace MinSG {

//! (ctor)
GeometryNode::GeometryNode():Node(){
	//ctor
}

//! (ctor)
GeometryNode::GeometryNode(const Util::Reference<Rendering::Mesh> & _mesh):Node() {
	setMesh(_mesh);
	//ctor
}

//! (ctor)
GeometryNode::GeometryNode(const GeometryNode &source):Node(source) {
	setMesh(source.getMesh());
	//ctor
}

//! (dtor)
GeometryNode::~GeometryNode() = default;

void GeometryNode::setMesh(const Util::Reference<Rendering::Mesh> & newMesh) {
	mesh = newMesh;
	worldBBChanged();
}

//! ---|> [Node]
void GeometryNode::doDisplay(FrameContext & context,const RenderParam & rp) {
	if (rp.getFlag(BOUNDING_BOXES) ) {
		// worldBB
		Rendering::drawAbsWireframeBox(context.getRenderingContext(), getWorldBB(), Util::ColorLibrary::LIGHT_GREY);
		// BB
		Rendering::drawWireframeBox(context.getRenderingContext(), getBB(), Util::ColorLibrary::GREEN);
	}
	if (!mesh.isNull() && !(rp.getFlag(NO_GEOMETRY))) {
		context.getStatistics().countNode(this);
		context.displayMesh(mesh.get());
	}
}

//! ---|> [Node]
const Geometry::Box& GeometryNode::doGetBB() const {
	static const Geometry::Box emptyBB;
	return mesh.isNull() ? emptyBB : mesh->getBoundingBox();// geometryBB;
}

uint32_t GeometryNode::getTriangleCount() const {
	if(mesh.isNull() || mesh->getDrawMode() != Rendering::Mesh::DRAW_TRIANGLES) {
		return 0;
	}
	return mesh->getPrimitiveCount();
}

uint32_t GeometryNode::getVertexCount()const{
	return mesh.isNull() ? 0 : mesh->getVertexCount();
}

size_t GeometryNode::getMemoryUsage() const {
	return Node::getMemoryUsage() - sizeof(Node) + sizeof(GeometryNode);
}

}
