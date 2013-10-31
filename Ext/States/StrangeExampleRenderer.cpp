/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "StrangeExampleRenderer.h"

#include "../../Core/Nodes/GeometryNode.h"
#include "../../Core/FrameContext.h"
#include "../../Helper/StdNodeVisitors.h"
#include <Geometry/Box.h>
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Util/Macros.h>
#include <Util/Timer.h>

namespace MinSG {

/**
 * [ctor]
 */
StrangeExampleRenderer::StrangeExampleRenderer():State() {
	//ctor
}

/**
 * [dtor]
 */
StrangeExampleRenderer::~StrangeExampleRenderer() {
	//dtor
}

/**
 * ---|> [State]
 */
StrangeExampleRenderer * StrangeExampleRenderer::clone()const {
	return new StrangeExampleRenderer();
}


/**
 * ---|> [State]
 */
State::stateResult_t  StrangeExampleRenderer::doEnableState(FrameContext & context,Node * node, const RenderParam & rp){

	if (rp.getFlag(NO_GEOMETRY) || rp.getFlag(SKIP_RENDERER))
		return State::STATE_SKIPPED;

	/// Collect all nodes in the frustum
	const auto nodes = collectNodesInFrustum<GeometryNode>(node, context.getCamera()->getFrustum());

	if(nodes.size() == 1 && nodes.front() == node) {
		WARN("This renderer must not be attached to GeometryNodes.");
		return State::STATE_SKIPPED;
	}

	/// Render all nodes with a little extra strangeness
	Geometry::Matrix4x4 m;
	float f=0;
	for(const auto & geoNode : nodes) {
		float f2=f+Util::Timer::now();
		m.setIdentity();
		m.scale(1.0+sin( f2)/40.0);
		m.rotate_deg( sin( f2/0.7)/2.0,0,1,0);
		context.getRenderingContext().pushMatrix();
		context.getRenderingContext().multMatrix(m);
		context.displayNode(geoNode,rp);
		context.getRenderingContext().popMatrix();
		f+=137.0;

	}

	return State::STATE_SKIP_RENDERING;
}

}
