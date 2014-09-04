/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "CameraNode.h"
#include "../Transformations.h"

using namespace Geometry;
namespace MinSG {

CameraNode::CameraNode():
		AbstractCameraNode(),fovLeft(Geometry::Angle::deg(-30)),fovRight(Geometry::Angle::deg(30)),
		fovBottom(Geometry::Angle::deg(-30)),fovTop(Geometry::Angle::deg(30)) {
	updateFrustum();
}

CameraNode::CameraNode(const CameraNode & o) :
		AbstractCameraNode(o),fovLeft(o.fovLeft),fovRight(o.fovRight),fovBottom(o.fovBottom),fovTop(o.fovTop){
	updateFrustum();
}

void CameraNode::updateFrustum() {
	frustum.setFrustumFromAngles(fovLeft,fovRight,fovBottom,fovTop,getNearPlane(),getFarPlane());
	frustum.setPosition( 	getWorldOrigin(), // pos
							Transformations::localDirToWorldDir(*this,Geometry::Vec3(0,0,-1)),  	// dir
							Transformations::localDirToWorldDir(*this,Geometry::Vec3(0,1,0)));	// up
}

void CameraNode::applyHorizontalAngle(const Geometry::Angle & angle) {
	const Geometry::Angle halfHAngle = angle * 0.5f;
	const float aspectRatio = static_cast<float>(getHeight()) / static_cast<float>(getWidth());
	const Geometry::Angle halfVAngle = Geometry::Angle::rad(std::atan(aspectRatio * std::tan(halfHAngle.rad())));
	setAngles(-halfHAngle, halfHAngle, -halfVAngle, halfVAngle);
}

void CameraNode::applyVerticalAngle(const Geometry::Angle & angle) {
	const Geometry::Angle halfVAngle = angle * 0.5f;
	const float aspectRatio = static_cast<float>(getWidth()) / static_cast<float>(getHeight());
	const Geometry::Angle halfHAngle = Geometry::Angle::rad(std::atan(aspectRatio * std::tan(halfVAngle.rad())));
	setAngles(-halfHAngle, halfHAngle, -halfVAngle, halfVAngle);
}

void CameraNode::setAngles(const Geometry::Angle & _fovLeft, const Geometry::Angle & _fovRight, 
						const Geometry::Angle & _fovBottom, const Geometry::Angle & _fovTop) {
	fovLeft = _fovLeft;
	fovRight = _fovRight;
	fovBottom = _fovBottom;
	fovTop = _fovTop;
	updateFrustum();
}

}
