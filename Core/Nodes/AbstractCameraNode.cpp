/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "AbstractCameraNode.h"
#include "../FrameContext.h"
#include <Geometry/Matrix4x4.h>
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Rendering/DrawCompound.h>
#include <Util/Graphics/ColorLibrary.h>

namespace MinSG {


static void updateCameraFrustum(Node * camera){
	static_cast<AbstractCameraNode*>(camera)->updateFrustum();
}

AbstractCameraNode::AbstractCameraNode() :
		Node(), viewport(0, 0, 1024, 768), nearPlane(0.1f), farPlane(10000.0f), scissorRect(), scissorEnabled(false) {
	addTransformationObserver(updateCameraFrustum);
}

AbstractCameraNode::AbstractCameraNode(const AbstractCameraNode & o) :
		Node(o), viewport(o.viewport), nearPlane(o.nearPlane), farPlane(o.farPlane), scissorRect(o.scissorRect), scissorEnabled(o.scissorEnabled) {
	addTransformationObserver(updateCameraFrustum);
}

AbstractCameraNode::~AbstractCameraNode() {
}

void AbstractCameraNode::setNearFar(float near, float far) {
	nearPlane = near;
	farPlane = far;
	updateFrustum();
}

void AbstractCameraNode::doDisplay(FrameContext & context, const RenderParam & rp) {
	if (rp.getFlag(SHOW_META_OBJECTS)) {
		Rendering::RenderingContext & renderingContext = context.getRenderingContext();
		renderingContext.pushMatrix();
		renderingContext.resetMatrix();
		Rendering::drawFrustum(renderingContext, frustum, Util::ColorLibrary::WHITE, 1.0);
		renderingContext.popMatrix();
		Rendering::drawCamera(renderingContext, Util::ColorLibrary::RED);
	}
}

}
