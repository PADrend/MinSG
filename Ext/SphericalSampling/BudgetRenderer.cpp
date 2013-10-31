/*
	This file is part of the MinSG library extension SphericalSampling.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_SPHERICALSAMPLING

#include "BudgetRenderer.h"
#include "../../Core/Nodes/GeometryNode.h"
#include "../../Core/FrameContext.h"
#include <Geometry/Rect.h>
#include <Rendering/Mesh/Mesh.h>
#include <Rendering/RenderingContext/RenderingContext.h>
#include <algorithm>
#include <cstdint>
#include <utility>
#include <vector>

namespace MinSG {
namespace SphericalSampling {

const Util::StringIdentifier BudgetRenderer::projectedSizeId("ProjectedSize");

BudgetRenderer::BudgetRenderer() : 
	NodeRendererState(FrameContext::DEFAULT_CHANNEL),
	budget(1000000),
	collectedNodes() {
}

NodeRendererResult BudgetRenderer::displayNode(FrameContext & /*context*/, Node * node, const RenderParam & /*rp*/) {
	GeometryNode * geoNode = dynamic_cast<GeometryNode *>(node);
	if(geoNode != nullptr) {
		collectedNodes.emplace_back(0.0, geoNode);
		return NodeRendererResult::NODE_HANDLED;
	}
	return NodeRendererResult::PASS_ON;
}

State::stateResult_t BudgetRenderer::doEnableState(FrameContext & context, Node * node, const RenderParam & rp) {
	collectedNodes.clear();
	return NodeRendererState::doEnableState(context, node, rp);
}

static uint32_t getPrimitiveCount(GeometryNode * geoNode) {
	if(geoNode == nullptr) {
		return 0;
	}
	const auto mesh = geoNode->getMesh();
	if(mesh == nullptr) {
		return 0;
	}
	return mesh->getPrimitiveCount();
}

void BudgetRenderer::doDisableState(FrameContext & context, Node * node, const RenderParam & rp) {
	NodeRendererState::doDisableState(context, node, rp);

	const Geometry::Rect_f screenRect(context.getRenderingContext().getWindowClientArea());
	for(auto & ratioNodePair : collectedNodes) {
		auto geoNode = ratioNodePair.second;

		// Clip the projected rect to the screen.
		auto projRect = context.getProjectedRect(geoNode);
		projRect.clipBy(screenRect);
		double projSize = projRect.getArea();

		// Check for the existence of a projected size attribute
		const auto projSizeAttribute = geoNode->getAttribute(projectedSizeId);
		if(projSizeAttribute != nullptr) {
			projSize = std::min(projSize, projSizeAttribute->toDouble());
		}

		ratioNodePair.first = getPrimitiveCount(geoNode) / projSize;
	}
	std::sort(collectedNodes.begin(), collectedNodes.end());

	uint32_t budgetUsed = 0;
	for(const auto & ratioNodePair : collectedNodes) {
		auto geoNode = ratioNodePair.second;
		const auto primitiveCount = getPrimitiveCount(geoNode);

		if(budgetUsed + primitiveCount > budget) {
			break;
		}
		context.displayNode(geoNode, rp + USE_WORLD_MATRIX);
		budgetUsed += primitiveCount;
	}
}

BudgetRenderer * BudgetRenderer::clone() const {
	return new BudgetRenderer(*this);
}

}
}

#endif /* MINSG_EXT_SPHERICALSAMPLING */
