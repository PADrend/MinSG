/*
	This file is part of the MinSG library extension MultiAlgoRendering.
	Copyright (C) 2012-2013 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_MULTIALGORENDERING

#include "SurfelRenderer.h"
#include "../../Core/Nodes/GroupNode.h"
#include "../../Core/Nodes/AbstractCameraNode.h"
#include <Geometry/Tools.h>
#include <Rendering/Mesh/Mesh.h>
#include <Rendering/RenderingContext/RenderingParameters.h>
#include <Rendering/RenderingContext/RenderingContext.h>

namespace MinSG {
namespace MAR {

float SurfelRenderer::getProjSize(FrameContext & context, Node * node){
	return context.getProjectedRect(node).getArea();
}

NodeRendererResult SurfelRenderer::displayNode(FrameContext & context, Node * node, const RenderParam & /*rp*/) {

	Rendering::Mesh * surfels = getSurfels(node);

	if(surfels != nullptr) {

		float projectedSize = getProjSize(context, node);
		if(projectedSize <= 0)
			return NodeRendererResult::NODE_HANDLED;
		float needed = projectedSize * getSurfelCoverage(node) * getSurfelCountFactor();
		float available = static_cast<float>(surfels->getPrimitiveCount());

		if(needed <= available) { // good case, enough surfels available
			displaySurfels(context, surfels, node->getWorldMatrix(), needed, getSurfelSizeFactor());
			return NodeRendererResult::NODE_HANDLED;
		} else if(getForceSurfels() && !dynamic_cast<GroupNode *>(node)) { // bad case 1: forced, less surfels available but leaf
			displaySurfels(context, surfels, node->getWorldMatrix(), available, std::min(maxSurfelSize, std::sqrt(needed/available)) * getSurfelSizeFactor());
			return NodeRendererResult::NODE_HANDLED;
		}
	}

	GroupNode * gn = dynamic_cast<GroupNode *>(node);
	if(gn) { // bad case 2: no or (less surfels available and groupnode) --> try downwards
		return NodeRendererResult::PASS_ON;
	} else {// bad case 3: no surfels available and leave
		if(getForceSurfels()) {  // forced --> try upwards
			while(node->hasParent()) {
				node = node->getParent();
				if(hasSurfels(node)) {
					displayOnDeaktivate.insert(node);
					return NodeRendererResult::NODE_HANDLED; // success --> display parent surfels
				}
			}
			//WARN("absolutely no surfels available to display this node");
			return NodeRendererResult::NODE_HANDLED; // fail --> display nothing
		} else { // unforced --> display geomery
			return NodeRendererResult::PASS_ON;
		}
	}
}

void SurfelRenderer::displaySurfels(FrameContext & context, Rendering::Mesh * surfelMesh, Geometry::Matrix4x4f worldMatrix, float surfelCount, float surfelSize) {
	FAIL_IF(surfelSize <= 0);
	context.getRenderingContext().pushAndSetMatrix_modelToCamera( context.getRenderingContext().getMatrix_worldToCamera() );
	context.getRenderingContext().multMatrix_modelToCamera(worldMatrix);
	context.getRenderingContext().pushAndSetPointParameters(Rendering::PointParameters(surfelSize));
	surfelCount = std::max(surfelCount, 1.0f);
	context.displayMesh(surfelMesh, 0, surfelCount);
	context.getRenderingContext().popPointParameters();
	context.getRenderingContext().popMatrix_modelToCamera();
}

Rendering::Mesh * SurfelRenderer::getSurfels(Node * node) {
	static const Util::StringIdentifier surfelId("surfels");
	auto surfelContainer = dynamic_cast<Util::ReferenceAttribute<Rendering::Mesh>*>(node->findAttribute(surfelId));
	return surfelContainer != nullptr ? surfelContainer->get() : nullptr;
}

float SurfelRenderer::getSurfelCoverage(Node * node) { //return 0.5f;
	static const Util::StringIdentifier surfelCoveringId("surfelRelCovering");
	auto surfelCoverage = node->findAttribute(surfelCoveringId);
	return surfelCoverage != nullptr ? surfelCoverage->toFloat() : (WARN("crazy"),0.0f);
}

void SurfelRenderer::doDisableState(FrameContext & context, Node * node, const RenderParam & rp) {

	for(const auto & n : displayOnDeaktivate) {
		Rendering::Mesh * surfels = getSurfels(n);
		float projectedSize = getProjSize(context, n);
		float needed = projectedSize * getSurfelCoverage(n) * getSurfelCountFactor();
		float available = static_cast<float>(surfels->getPrimitiveCount());
		displaySurfels(context, surfels, n->getWorldMatrix(), available, std::min(maxSurfelSize, std::sqrt(needed/available)) * getSurfelSizeFactor());
	}
	displayOnDeaktivate.clear();

	return NodeRendererState::doDisableState(context, node, rp);
}

}
}

#endif // MINSG_EXT_MULTIALGORENDERING
