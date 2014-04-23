/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "BillboardNode.h"
#include "../../Core/Nodes/AbstractCameraNode.h"
#include "../../Core/Nodes/GroupNode.h"
#include "../../Core/FrameContext.h"
#include <Rendering/Mesh/Mesh.h>
#include <Rendering/MeshUtils/MeshBuilder.h>
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Util/Macros.h>
#include <Util/StringUtils.h>

namespace MinSG {
using Geometry::Rect;

BillboardNode::BillboardNode(Geometry::Rect _rect, bool _rotateUpAxis, bool _rotateRightAxis) :
	GeometryNode(), rotateUpAxis(_rotateUpAxis), rotateRightAxis(_rotateRightAxis), rect(std::move(_rect)) {
}

BillboardNode::~BillboardNode() = default;


/// ---|> [Node]
void BillboardNode::doDisplay(FrameContext & context,const RenderParam & rp){
	using namespace Geometry;

	AbstractCameraNode * cam=context.getCamera();

	if(cam==nullptr){
		GeometryNode::doDisplay(context,rp);
		return;
	}
	const Frustum & f=cam->getFrustum();

	Vec3 dir;
	if(rotateRightAxis){
		dir=getWorldPosition()-f.getPos();
		dir.normalize();
	}else{
		dir=f.getDir();
	}
	Vec3 up;
	if(rotateUpAxis){
		up=f.getUp();
	}else{
		up = context.getWorldUpVector();
		dir.x( dir.x() * std::max(0.0001f, -up.x() + 1) );
		dir.y( dir.y() * std::max(0.0001f, -up.y() + 1) );
		dir.z( dir.z() * std::max(0.0001f, -up.z() + 1) );
		dir.normalize();
	}

	// TODO here we have got a problem when using lots of Billboard nodes
	// this should be reimplemented without inverting matrices

	if(hasParent() && !getParent()->getWorldMatrix().isIdentity()){
		Matrix4x4f iwm = getParent()->getWorldMatrix().inverse();
		dir = iwm.transformDirection(dir);
		up = iwm.transformDirection(up);
	}

	accessSRT().setRotation(dir,up);
	transformationChanged();

	if(getMesh()==nullptr)
		createMesh();

	context.getRenderingContext().pushMatrix();
	context.getRenderingContext().resetMatrix();
	context.getRenderingContext().multMatrix(getWorldMatrix());

	GeometryNode::doDisplay(context,rp);

	context.getRenderingContext().popMatrix();
}

///
void BillboardNode::createMesh(){
	Rendering::VertexDescription vertexDescription;
	vertexDescription.appendPosition3D();
	vertexDescription.appendNormalByte();
	vertexDescription.appendTexCoord();
	Rendering::MeshUtils::MeshBuilder mb(vertexDescription);
	mb.position( Geometry::Vec3(rect.getMinX(),rect.getMinY(),0));
	mb.normal( Geometry::Vec3(0,0,-1.0f) );
	mb.texCoord0( Geometry::Vec2(1.0f,0) );
	mb.addVertex();

	mb.position( Geometry::Vec3(rect.getMinX(),rect.getMaxY(),0));
	mb.normal( Geometry::Vec3(0,0,-1.0f) );
	mb.texCoord0( Geometry::Vec2(1.0f,1.0f) );
	mb.addVertex();

	mb.position( Geometry::Vec3(rect.getMaxX(),rect.getMaxY(),0));
	mb.normal( Geometry::Vec3(0,0,-1.0f) );
	mb.texCoord0( Geometry::Vec2(0,1.0f) );
	mb.addVertex();

	mb.position( Geometry::Vec3(rect.getMaxX(),rect.getMinY(),0));
	mb.normal( Geometry::Vec3(0,0,-1.0f) );
	mb.texCoord0( Geometry::Vec2(0,0) );
	mb.addVertex();

	mb.addQuad(0,1,2,3);

	setMesh(mb.buildMesh());
}

void BillboardNode::setRect(Geometry::Rect _rect){
    rect = std::move(_rect);
    createMesh();
}

}
