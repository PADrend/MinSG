/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_VISIBILITY_SUBDIVISION
#ifdef MINSG_EXT_EVALUATORS

#include "CostEvaluator.h"
#include "VisibilityVector.h"
#include "../../Core/Nodes/GeometryNode.h"
#include "../../Core/FrameContext.h"
#include "../../Helper/StdNodeVisitors.h"
#include "../../Helper/VisibilityTester.h"
#include <Geometry/Box.h>
#include <Rendering/RenderingContext/RenderingParameters.h>
#include <Rendering/RenderingContext/RenderingContext.h>
#include <algorithm>
#include <functional>

namespace MinSG {
namespace VisibilitySubdivision {

CostEvaluator::CostEvaluator(DirectionMode _mode) :
	Evaluator(_mode) {
	setMaxValue_i(0u);
}

CostEvaluator::~CostEvaluator() {
}

void CostEvaluator::beginMeasure() {
	values->clear();
	setMaxValue_i(0u);
}

void CostEvaluator::measure(FrameContext & context, Node & node, const Geometry::Rect & /*r*/) {
	// Determine the possibly visible nodes inside the frustum.
	auto objectsInFrustum = collectNodesInFrustum<GeometryNode>(&node, context.getCamera()->getFrustum());

	/*
	 * Remove inactive nodes. Inactive nodes are ignored in Node::display()
	 * anyway, but removing them here makes visibility testing a little bit
	 * faster when testing a large number of nodes.
	 */
	objectsInFrustum.erase(
		std::remove_if(objectsInFrustum.begin(), objectsInFrustum.end(), [](const Node * obj) { return !obj->isActive(); }),
		objectsInFrustum.end()
	);

	// Disable color writes
	context.getRenderingContext().pushAndSetColorBuffer(Rendering::ColorBufferParameters(false, false, false, false));
	// Enable depth writes
	context.getRenderingContext().pushAndSetDepthBuffer(Rendering::DepthBufferParameters(true, true, Rendering::Comparison::LEQUAL));
	// Disable face culling
	context.getRenderingContext().pushAndSetCullFace(Rendering::CullFaceParameters());
	context.getRenderingContext().applyChanges();

	context.getRenderingContext().clearDepth(1.0f);
	// First pass: Render scene to fill depth buffer.
	for(const auto & geoNode : objectsInFrustum) {
		geoNode->display(context, USE_WORLD_MATRIX | NO_STATES);
	}

	context.getRenderingContext().popCullFace();
	context.getRenderingContext().popDepthBuffer();
	context.getRenderingContext().popColorBuffer();

	// Make sure that the depth test is enabled. Otherwise, all samples will pass it and all objects will be visible.
	context.getRenderingContext().pushAndSetDepthBuffer(Rendering::DepthBufferParameters(true, true, Rendering::Comparison::LEQUAL));
	auto visibilityResults = VisibilityTester::testNodes(context, objectsInFrustum);
	context.getRenderingContext().popDepthBuffer();

	if(values->empty() && mode == SINGLE_VALUE) {
		// Create the only element that is needed for this mode.
		values->push_back(new VisibilitySubdivision::VisibilityVectorAttribute);
	}
	if(mode == DIRECTION_VALUES) {
		// Create a new element at the end of the list.
		values->push_back(new VisibilitySubdivision::VisibilityVectorAttribute);
	}

	// For mode SINGLE_VALUE this is the only element in the list.
	auto & vv = dynamic_cast<VisibilitySubdivision::VisibilityVectorAttribute *>(values->back())->ref();

	uint32_t trianglesCount = getMaxValue()->toUnsignedInt();
	for(const auto & result : visibilityResults) {
		vv.setNode(result.first, result.second);
		trianglesCount += result.first->getTriangleCount();
	}
	setMaxValue_i(trianglesCount);
}

void CostEvaluator::endMeasure(FrameContext &) {
}

}
}

#endif // MINSG_EXT_EVALUATORS
#endif // MINSG_EXT_VISIBILITY_SUBDIVISION
