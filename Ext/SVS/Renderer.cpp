/*
	This file is part of the MinSG library extension SVS.
	Copyright (C) 2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_SVS

#include "Renderer.h"
#include "BudgetRenderer.h"
#include "Helper.h"
#include "VisibilitySphere.h"
#include "../VisibilitySubdivision/VisibilityVector.h"
#include "../../Core/Nodes/GeometryNode.h"
#include "../../Core/Nodes/GroupNode.h"
#include "../../Core/Nodes/Node.h"
#include "../../Core/FrameContext.h"
#include "../../Helper/DistanceSorting.h"
#include "../../Helper/FrustumTest.h"
#include "../../Helper/StdNodeVisitors.h"
#include <Geometry/Sphere.h>
#include <Geometry/Vec3.h>
#include <Rendering/RenderingContext/RenderingParameters.h>
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Rendering/Draw.h>
#include <Rendering/OcclusionQuery.h>
#include <Util/GenericAttribute.h>
#include <Util/Macros.h>
#include <Util/References.h>
#include <algorithm>
#include <deque>
#include <cstdint>
#include <numeric>
#include <random>

#ifdef MINSG_PROFILING
#include "Statistics.h"
#include "../../Core/Statistics.h"
#endif /* MINSG_PROFILING */

namespace MinSG {
namespace SVS {

Renderer::Renderer() : 
	NodeRendererState(FrameContext::DEFAULT_CHANNEL), 
	interpolationMethod(INTERPOLATION_MAX3),
	performSphereOcclusionTest(false),
	performGeometryOcclusionTest(false) {
}

NodeRendererResult Renderer::displayNode(FrameContext & context, Node * node, const RenderParam & rp) {
	try {
		if(!node->isActive()) {
			return NodeRendererResult::NODE_HANDLED;
		}

		GeometryNode * geoNode = dynamic_cast<GeometryNode *>(node);
		if(geoNode != nullptr) {
			return NodeRendererResult::PASS_ON;
		}

		GroupNode * groupNode = dynamic_cast<GroupNode *>(node);
		if(groupNode == nullptr) {
			WARN(std::string("Cannot handle unknown node type \"") + node->getTypeName() + "\".");
			return NodeRendererResult::PASS_ON;
		}

		// Simply ignore nodes without a visibility sphere.
		if(!hasVisibilitySphere(groupNode)) {
			return NodeRendererResult::PASS_ON;
		}

		const VisibilitySphere & visibilitySphere = retrieveVisibilitySphere(groupNode);
		const auto & sphere = visibilitySphere.getSphere();
		const auto worldSphere = transformSphere(sphere, groupNode->getWorldTransformationMatrix());
#ifdef MINSG_PROFILING
		++numSpheresVisited;
#endif /* MINSG_PROFILING */

		const Geometry::Vec3f cameraPos = context.getCamera()->getWorldOrigin();
		if(!worldSphere.isOutside(cameraPos)) {
#ifdef MINSG_PROFILING
			++numSpheresEntered;
#endif /* MINSG_PROFILING */

			// Perform front-to-back rendering only if occlusion tests are used.
			if(performSphereOcclusionTest || performGeometryOcclusionTest) {
				const auto & frustum = context.getCamera()->getFrustum();
				// Sort the spheres/geometry front-to-back.
				NodeDistancePriorityQueue_F2B distanceQueue(cameraPos);
				const auto children = getChildNodes(groupNode);
				for(const auto & child : children) {
					if(conditionalFrustumTest(frustum, child->getWorldBB(), rp)) {
						distanceQueue.push(child);
					}
				}
				while(!distanceQueue.empty()) {
					context.displayNode(distanceQueue.top(), rp + USE_WORLD_MATRIX);
					distanceQueue.pop();
				}
				return NodeRendererResult::NODE_HANDLED;
			} else {
				// Use standard traversal.
				return NodeRendererResult::PASS_ON;
			}
		}

		if(performGeometryOcclusionTest) {
			processPendingGeometryQueries(context, rp);
		}
		if(performSphereOcclusionTest && !groupNode->getWorldBB().contains(cameraPos)) {
			processPendingSphereQueries(context, rp);
			testSphere(context, groupNode);
		} else {
			displaySphere(context, groupNode, rp, false);
		}
	} catch(const std::exception & e) {
		WARN(std::string("Exception during rendering: ") + e.what());
		deactivate();

		// Do this here because doDisableState is not called for inactive states.
		doDisableState(context, node, rp);
	}

	return NodeRendererResult::NODE_HANDLED;
}

static void setOrUpdateAttribute(Node * node, const Util::StringIdentifier & attributeId, double value) {
	const auto numberAttribute = dynamic_cast<Util::_NumberAttribute<double> *>(node->getAttribute(attributeId));
	if(numberAttribute != nullptr) {
		numberAttribute->set(value);
	} else {
		node->setAttribute(attributeId, Util::GenericAttribute::createNumber(value));
	}
}

void Renderer::displaySphere(FrameContext & context, GroupNode * groupNode, const RenderParam & rp, bool skipGeometryOcclusionTest) {
	const VisibilitySphere & visibilitySphere = retrieveVisibilitySphere(groupNode);
	const Geometry::Sphere_f & sphere = visibilitySphere.getSphere();
	const auto worldCenter = groupNode->getWorldTransformationMatrix().transformPosition(sphere.getCenter());

	const Geometry::Vec3f cameraPos = context.getCamera()->getWorldOrigin();
	const Geometry::Vec3f direction = (cameraPos - worldCenter).getNormalized();
	const auto vv = visibilitySphere.queryValue(direction, interpolationMethod);
	const uint32_t maxIndex = vv.getIndexCount();

	const auto & frustum = context.getCamera()->getFrustum();

	if(!skipGeometryOcclusionTest && performGeometryOcclusionTest) {
		_GenericDistancePriorityQueue<GeometryNode *, DistanceCalculators::NodeDistanceCalculator, std::less> distanceQueue(cameraPos);

		for(uint_fast32_t index = 0; index < maxIndex; ++index) {
			if(vv.getBenefits(index) == 0) {
				continue;
			}
			auto potentiallyVisibleNode = vv.getNode(index);
			if(conditionalFrustumTest(frustum, potentiallyVisibleNode->getWorldBB(), rp)) {
				distanceQueue.push(potentiallyVisibleNode);
			}
		}
		
		while(!distanceQueue.empty()) {
			testGeometry(context, distanceQueue.top());
			distanceQueue.pop();
		}
	} else {
		typedef VisibilitySubdivision::VisibilityVector::benefits_t benefits_t;
		std::vector<std::pair<GeometryNode *, benefits_t>> pvNodes;
		benefits_t overallBenefits = 0;

		for(uint_fast32_t index = 0; index < maxIndex; ++index) {
			const auto benefits = vv.getBenefits(index);
			if(benefits == 0) {
				continue;
			}
			auto potentiallyVisibleNode = vv.getNode(index);
			if(conditionalFrustumTest(frustum, potentiallyVisibleNode->getWorldBB(), rp)) {
				pvNodes.push_back(std::make_pair(potentiallyVisibleNode, benefits));
				overallBenefits += benefits;
			}
		}

		if(pvNodes.empty()) {
			return;
		}

		const double benefitsNormalizer = 1.0 / static_cast<double>(overallBenefits);

		const Geometry::Rect_f screenRect(context.getRenderingContext().getWindowClientArea());

		// Clip the projected rect to the screen.
		auto projRect = context.getProjectedRect(groupNode);
		projRect.clipBy(screenRect);
		const auto projSize = projRect.getArea();

		for(const auto & pvNodeBenefitsPair : pvNodes) {
			setOrUpdateAttribute(pvNodeBenefitsPair.first,
									BudgetRenderer::projectedSizeId,
									pvNodeBenefitsPair.second * benefitsNormalizer * projSize);
			context.displayNode(pvNodeBenefitsPair.first, rp + USE_WORLD_MATRIX);
		}
	}
}

void Renderer::testSphere(FrameContext & context, GroupNode * groupNode) {
#ifdef MINSG_PROFILING
	MinSG::Statistics & statistics = context.getStatistics();
	statistics.pushEvent(MinSG::Statistics::EVENT_TYPE_START_TEST, 1);
#endif /* MINSG_PROFILING */

	// Perform visibility test
	sphereQueries.emplace_back(	std::shared_ptr<Rendering::OcclusionQuery>(new Rendering::OcclusionQuery), 
								std::shared_ptr<Rendering::OcclusionQuery>(new Rendering::OcclusionQuery), 
								groupNode);
	Rendering::OcclusionQuery * occQuery = std::get<0>(sphereQueries.back()).get();
	Rendering::OcclusionQuery * fullBoxOccQuery = std::get<1>(sphereQueries.back()).get();

	Rendering::RenderingContext & renderingContext = context.getRenderingContext();

	// Second pass: Determine the visible bounding boxes first.
	Rendering::OcclusionQuery::enableTestMode(renderingContext);

	renderingContext.pushAndSetCullFace(Rendering::CullFaceParameters(Rendering::CullFaceParameters::CULL_BACK));
	// Use polygon offset when rendering bounding boxes to prevent errors due to z fighting.
	renderingContext.pushAndSetPolygonOffset(Rendering::PolygonOffsetParameters(0.0f, -1.0f));
	renderingContext.applyChanges();

	occQuery->begin();
	Rendering::drawAbsBox(renderingContext, groupNode->getWorldBB());
	occQuery->end();

	// Render the box again without depth test to determine the overall number of samples.
	renderingContext.pushAndSetDepthBuffer(Rendering::DepthBufferParameters(false, false, Rendering::Comparison::LESS));
	renderingContext.applyChanges();

	fullBoxOccQuery->begin();
	Rendering::drawAbsBox(renderingContext, groupNode->getWorldBB());
	fullBoxOccQuery->end();

	renderingContext.popDepthBuffer();

	renderingContext.popPolygonOffset();
	renderingContext.popCullFace();
	Rendering::OcclusionQuery::disableTestMode(renderingContext);
}

bool Renderer::processPendingSphereQueries(FrameContext & context, const RenderParam & rp) {
#ifdef MINSG_PROFILING
	MinSG::Statistics & statistics = context.getStatistics();
#endif /* MINSG_PROFILING */
	while(!sphereQueries.empty()) {
		const auto & occQuery = std::get<0>(sphereQueries.front());
		if(!occQuery->isResultAvailable()) {
			return false;
		}
		const uint32_t sampleCount = occQuery->getResult();

		if(sampleCount == 0) {
			// Node is completely occluded
#ifdef MINSG_PROFILING
			statistics.pushEvent(MinSG::Statistics::EVENT_TYPE_END_TEST_INVISIBLE, 1);
#endif /* MINSG_PROFILING */
			sphereQueries.pop_front();
			continue;
		}

#ifdef MINSG_PROFILING
		statistics.pushEvent(MinSG::Statistics::EVENT_TYPE_END_TEST_VISIBLE, 1);
#endif /* MINSG_PROFILING */

		const auto & fullBoxOccQuery = std::get<1>(sphereQueries.front());
		const float visibilityRatio = static_cast<float>(sampleCount) / static_cast<float>(fullBoxOccQuery->getResult());
		const bool skipGeometryOcclusionTest = visibilityRatio > 0.75f;

		GroupNode * groupNode = std::get<2>(sphereQueries.front());

		displaySphere(context, groupNode, rp, skipGeometryOcclusionTest);

		sphereQueries.pop_front();
	}
	return true;
}

void Renderer::testGeometry(FrameContext & context, GeometryNode * geometryNode) {
#ifdef MINSG_PROFILING
	MinSG::Statistics & statistics = context.getStatistics();
	statistics.pushEvent(MinSG::Statistics::EVENT_TYPE_START_TEST, 1);
#endif /* MINSG_PROFILING */

	geometryQueries.emplace_back(std::shared_ptr<Rendering::OcclusionQuery>(new Rendering::OcclusionQuery), geometryNode);
	Rendering::OcclusionQuery * occQuery = geometryQueries.back().first.get();

	Rendering::RenderingContext & renderingContext = context.getRenderingContext();

	// Second pass: Determine the visible bounding boxes first.
	Rendering::OcclusionQuery::enableTestMode(renderingContext);

	renderingContext.pushAndSetCullFace(Rendering::CullFaceParameters());
	// Use polygon offset when rendering bounding boxes to prevent errors due to z fighting.
	renderingContext.pushAndSetPolygonOffset(Rendering::PolygonOffsetParameters(0.0f, -1.0f));
	renderingContext.applyChanges();

	occQuery->begin();
	Rendering::drawAbsBox(renderingContext, geometryNode->getWorldBB());
	occQuery->end();

	renderingContext.popPolygonOffset();
	renderingContext.popCullFace();
	Rendering::OcclusionQuery::disableTestMode(renderingContext);
}

bool Renderer::processPendingGeometryQueries(FrameContext & context, const RenderParam & rp) {
#ifdef MINSG_PROFILING
	MinSG::Statistics & statistics = context.getStatistics();
#endif /* MINSG_PROFILING */
	while(!geometryQueries.empty()) {
		const auto & occQuery = geometryQueries.front().first;
		if(!occQuery->isResultAvailable()) {
			return false;
		}
		const uint32_t sampleCount = occQuery->getResult();

		if(sampleCount == 0) {
			// Node is completely occluded
#ifdef MINSG_PROFILING
			statistics.pushEvent(MinSG::Statistics::EVENT_TYPE_END_TEST_INVISIBLE, 1);
#endif /* MINSG_PROFILING */
			geometryQueries.pop_front();
			continue;
		}

#ifdef MINSG_PROFILING
		statistics.pushEvent(MinSG::Statistics::EVENT_TYPE_END_TEST_VISIBLE, 1);
#endif /* MINSG_PROFILING */

		GeometryNode * geoNode = geometryQueries.front().second;
		context.displayNode(geoNode, rp + USE_WORLD_MATRIX);

		geometryQueries.pop_front();
	}
	return true;
}

#ifdef MINSG_PROFILING
State::stateResult_t Renderer::doEnableState(FrameContext & context, Node * node, const RenderParam & rp) {
	numSpheresVisited = 0;
	numSpheresEntered = 0;
	return NodeRendererState::doEnableState(context, node, rp);
}
#endif /* MINSG_PROFILING */

void Renderer::doDisableState(FrameContext & context, Node * node, const RenderParam & rp) {
	NodeRendererState::doDisableState(context, node, rp);

	// Work unitl all occlusion queries are finished
	while(	(performSphereOcclusionTest && !processPendingSphereQueries(context, rp)) ||
			(performGeometryOcclusionTest && !processPendingGeometryQueries(context, rp))) {
	}

#ifdef MINSG_PROFILING
	MinSG::Statistics & statistics = context.getStatistics();
	statistics.addValue(Statistics::instance(statistics).getVisitedSpheresCounter(), numSpheresVisited);
	statistics.addValue(Statistics::instance(statistics).getEnteredSpheresCounter(), numSpheresEntered);
#endif /* MINSG_PROFILING */
}

Renderer * Renderer::clone() const {
	return new Renderer(*this);
}

}
}

#endif /* MINSG_EXT_SVS */
