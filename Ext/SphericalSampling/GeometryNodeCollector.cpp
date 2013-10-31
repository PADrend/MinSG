/*
	This file is part of the MinSG library extension SphericalSampling.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_SPHERICALSAMPLING

#include "GeometryNodeCollector.h"
#include "../../Core/Nodes/GeometryNode.h"
#include "../../Core/FrameContext.h"

namespace MinSG {
namespace SphericalSampling {

GeometryNodeCollector::GeometryNodeCollector() : 
	NodeRendererState(FrameContext::DEFAULT_CHANNEL),
	collectedNodes() {
}

NodeRendererResult GeometryNodeCollector::displayNode(FrameContext & /*context*/, Node * node, const RenderParam & /*rp*/) {
	GeometryNode * geoNode = dynamic_cast<GeometryNode *>(node);
	if(geoNode != nullptr) {
		collectedNodes.insert(geoNode);
	}
	return NodeRendererResult::PASS_ON;
}

State::stateResult_t GeometryNodeCollector::doEnableState(FrameContext & context, Node * node, const RenderParam & rp) {
	collectedNodes.clear();
	return NodeRendererState::doEnableState(context, node, rp);
}

GeometryNodeCollector * GeometryNodeCollector::clone() const {
	return new GeometryNodeCollector(*this);
}

}
}

#endif /* MINSG_EXT_SPHERICALSAMPLING */
