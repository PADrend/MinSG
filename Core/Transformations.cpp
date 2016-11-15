/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "Transformations.h"
#include "Nodes/Node.h"
#include "Nodes/GroupNode.h"

namespace MinSG{

namespace Transformations{

Geometry::Vec3 localDirToWorldDir(const Node & node, const Geometry::Vec3 & localDir){
	return node.getWorldTransformationMatrixPtr() ? node.getWorldTransformationMatrix().transformDirection(localDir) : localDir;
}
Geometry::Vec3 localDirToRelDir(const Node & node, const Geometry::Vec3 & localDir){
	return node.hasRelTransformation() ? node.getRelTransformationMatrix().transformDirection(localDir) : localDir;
}
Geometry::Vec3 localPosToWorldPos(const Node & node, const Geometry::Vec3 & localPos){
	return node.getWorldTransformationMatrixPtr() ? node.getWorldTransformationMatrix().transformPosition(localPos) : localPos;
}
Geometry::Vec3 localPosToRelPos(const Node & node, const Geometry::Vec3 & localPos){
	return node.hasRelTransformation() ? node.getRelTransformationMatrix().transformPosition(localPos) : localPos;
}
Geometry::Vec3 relDirToWorldDir(const Node & node, const Geometry::Vec3 & relDir){
	return node.hasParent() ? localDirToWorldDir(*node.getParent(),relDir) : relDir;
}
Geometry::Vec3 relDirToLocalDir(const Node & node, const Geometry::Vec3 & relDir){
	return node.hasRelTransformation() ? node.getRelTransformationMatrix().inverse().transformDirection(relDir) : relDir;
}
Geometry::Vec3 relPosToWorldPos(const Node & node, const Geometry::Vec3 & relPos){
	return node.hasParent() ? localPosToWorldPos(*node.getParent(),relPos) : relPos;
}
Geometry::Vec3 relPosToLocalPos(const Node & node, const Geometry::Vec3 & relPos){
	return node.hasRelTransformation() ? node.getRelTransformationMatrix().inverse().transformPosition(relPos) : relPos;
}
Geometry::Vec3 worldDirToLocalDir(const Node & node, const Geometry::Vec3 & worldDir){
	return node.getWorldTransformationMatrixPtr() ? node.getWorldTransformationMatrix().inverse().transformDirection(worldDir) : worldDir;
}
Geometry::Vec3 worldDirToRelDir(const Node & node, const Geometry::Vec3 & worldDir){
	return node.hasParent() ? worldDirToLocalDir(*node.getParent(),worldDir) : worldDir;
}
Geometry::Vec3 worldPosToLocalPos(const Node & node, const Geometry::Vec3 & worldPos){
	return node.getWorldTransformationMatrixPtr() ? node.getWorldTransformationMatrix().inverse().transformPosition(worldPos) : worldPos;
}
Geometry::Vec3 worldPosToRelPos(const Node & node, const Geometry::Vec3 & worldPos){
	return node.hasParent() ? worldPosToLocalPos(*node.getParent(),worldPos) : worldPos;
}


Geometry::SRT worldSRTToRelSRT(const Node & node, const Geometry::SRT & worldSRT){
	const auto parent = node.getParent();
	if(!parent || parent->getWorldTransformationMatrixPtr()==nullptr){	// the node is not in a transformed subtree
		return worldSRT;
	}else{
		const auto relPos = worldPosToLocalPos( *parent, worldSRT.getTranslation() ); 
		const auto relDir = worldDirToLocalDir( *parent, worldSRT.getDirVector() ); 
		const auto relUp = worldDirToLocalDir( *parent, worldSRT.getUpVector() ); 
		return Geometry::SRT( relPos,relDir,relUp,relDir.length()*worldSRT.getScale() ); 
	}
}


void rotateAroundLocalAxis(Node & node, const Geometry::Angle & angle,const Geometry::Line3 & localAxis){
	Transformations::rotateAroundRelAxis(node,angle,
								Geometry::Line3(
									Transformations::localPosToRelPos(node,localAxis.getOrigin()),
									Transformations::localDirToRelDir(node,localAxis.getDirection())));
}

void rotateAroundRelAxis(Node & node, const Geometry::Angle & angle, const Geometry::Line3 & relAxis){
	const bool useSRT = node.hasRelTransformationSRT() || !node.hasRelTransformation(); // node has matrix or no transformation at all.
	
	Geometry::Matrix4x4 matrix;
	matrix.translate(relAxis.getOrigin());
	matrix.rotate_deg(angle.deg(),relAxis.getDirection().getNormalized()); 
//	matrix.rotate_rad(angle.deg()*(M_PI/180.0),relAxis.getDirection().getNormalized()); // \todo does not work. Why???
	matrix.translate(-relAxis.getOrigin());
	matrix = matrix * node.getRelTransformationMatrix();
	
	if(useSRT)
		node.setRelTransformation(matrix._toSRT());
	else
		node.setRelTransformation(matrix);
}

void rotateAroundWorldAxis(Node & node, const Geometry::Angle & angle,const Geometry::Line3 & worldAxis){
	Transformations::rotateAroundRelAxis(node,angle,
								Geometry::Line3(
									Transformations::worldPosToRelPos(node,worldAxis.getOrigin()),
									Transformations::worldDirToRelDir(node,worldAxis.getDirection())));
}
void rotateToWorldDir(Node & node, const Geometry::Vec3 & worldDir){
	const Geometry::Vec3 relDir = worldDirToRelDir(node,worldDir).normalize();
	
	const Geometry::Vec3 relRight = relDir.cross( (std::abs(relDir.y())<0.99 ? Geometry::Vec3(0,1,0) : Geometry::Vec3(1,0,0))  ).normalize();
	const Geometry::Vec3 relUp = relDir.cross( -relRight );
	Geometry::SRT srt = node.getRelTransformationSRT();
	srt.setRotation(relDir,relUp);
	node.setRelTransformation( srt );
}

}

}
