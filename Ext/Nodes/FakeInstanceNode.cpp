/*
	This file is part of the MinSG library.
	Copyright (C) 2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "FakeInstanceNode.h"
#include "../../Core/Nodes/AbstractCameraNode.h"
#include "../../Core/FrameContext.h"
#include <Geometry/Box.h>

namespace MinSG {

FakeInstanceNode::~FakeInstanceNode() = default;

void FakeInstanceNode::doDisplay(FrameContext & frameContext, const RenderParam & rp) {
	if(fakePrototype.isNull()) {
		Node::doDisplay(frameContext, rp);
		return;
	}

	auto camera = static_cast<AbstractCameraNode*>(frameContext.getCamera()->clone());
	const Geometry::Matrix4x4f & cameraMatrix = frameContext.getCamera()->getWorldTransformationMatrix();
	camera->setRelTransformation(	fakePrototype->getWorldTransformationMatrix() 
						* getWorldToLocalMatrix() 
						* cameraMatrix);

	frameContext.pushAndSetCamera(camera);
	frameContext.displayNode(fakePrototype.get(), rp);
	frameContext.popCamera();
}

const Geometry::Box& FakeInstanceNode::doGetBB() const {
	if(fakePrototype.isNull()) {
		static const Geometry::Box emptyBB;
		return emptyBB;
	}
	return fakePrototype->getBB();
}

}
