/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_EVALUATORS

#include "BoxQualityEvaluator.h"
#include "../../Core/Nodes/GeometryNode.h"
#include "../../Core/FrameContext.h"
#include "../../Helper/StdNodeVisitors.h"
#include <Geometry/Box.h>
#include <Rendering/Draw.h>
#include <Rendering/OcclusionQuery.h>
#include <Rendering/RenderingContext/RenderingParameters.h>
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Rendering/Shader/Shader.h>
#include <Util/Graphics/Color.h>
#include <Util/GenericAttribute.h>
#include <list>

namespace MinSG {
namespace Evaluators {

//! [ctor]
BoxQualityEvaluator::BoxQualityEvaluator():Evaluator(Evaluator::SINGLE_VALUE) {
}

//! [dtor]
BoxQualityEvaluator::~BoxQualityEvaluator() {
}

//! ---|> Evaluator
void BoxQualityEvaluator::beginMeasure() {
	objectsClassifiedAsV.clear();
	objectsVisible.clear();
	objectsInVF.clear();
	values->clear();
	setMaxValue_i(0);
}

//! ---|> Evaluator
void BoxQualityEvaluator::measure(FrameContext & context,Node & node,const Geometry::Rect & /*r*/) {

	// first pass - fill depth buffer.
	context.getRenderingContext().clearScreen(Util::Color4f(0.9f, 0.9f, 0.9f, 1.0f));
	node.display(context,USE_WORLD_MATRIX|FRUSTUM_CULLING);



	if (getMaxValue()->toInt() == 0.0) {
		setMaxValue_i(static_cast<uint32_t>(collectNodes<GeometryNode>(&node).size()));
	}


	// collect geometry nodes in frustum
	auto objectsInVFList = collectNodesInFrustum<GeometryNode>(&node, context.getCamera()->getFrustum());
	for(const auto & geoNode : objectsInVFList) {
		objectsInVF[reinterpret_cast<uintptr_t>(geoNode)] = geoNode;
	}


	// setup occlusion queries
	size_t numQueries=objectsInVFList.size();
	if (numQueries==0) return;


	auto queries = new Rendering::OcclusionQuery[numQueries];

	{ // optional second pass: test bounding boxes (get objects that are classified as visible)
		context.getRenderingContext().pushAndSetDepthBuffer(Rendering::DepthBufferParameters(true, true, Rendering::Comparison::LEQUAL));
		uint32_t i=0;
		// start queries
		Rendering::OcclusionQuery::enableTestMode(context.getRenderingContext());
		for(const auto & geoNode : objectsInVFList) {
			queries[i].begin(context.getRenderingContext());
//				queries[i].fastBoxTest(n->getWorldBB());
			Rendering::drawAbsBox(context.getRenderingContext(), geoNode->getWorldBB() );
			queries[i].end(context.getRenderingContext());
			++i;
		}
		Rendering::OcclusionQuery::disableTestMode(context.getRenderingContext());
		i=0;
		// get results
		for(auto & geoNode : objectsInVFList) {
			if (queries[i].getResult(context.getRenderingContext()) == 0 && !geoNode->getWorldBB().contains(context.getCamera()->getWorldOrigin()) ) {
				geoNode = nullptr;
			} else {
				objectsClassifiedAsV[reinterpret_cast<uintptr_t>(geoNode)] = geoNode;
			}
			++i;
		}
		context.getRenderingContext().popDepthBuffer();
	}


	{ // third pass: test if nodes are exactly visible
		context.getRenderingContext().pushAndSetDepthBuffer(Rendering::DepthBufferParameters(true, true, Rendering::Comparison::EQUAL));
		uint32_t i=0;

		// start queries
		for(const auto & geoNode : objectsInVFList) {
			if (!geoNode)
				continue;
			queries[i].begin(context.getRenderingContext());
			// render object i
			context.displayNode(geoNode, USE_WORLD_MATRIX);
			queries[i].end(context.getRenderingContext());
			++i;
		}
		i=0;
		// get results
		for(const auto & geoNode : objectsInVFList) {
			if (!geoNode)
				continue;
			if (queries[i].getResult(context.getRenderingContext()) > 0) {
				objectsVisible[reinterpret_cast<uintptr_t>(geoNode)] = geoNode;
			}
			++i;
		}
		context.getRenderingContext().popDepthBuffer();
	}

	delete [] queries;
}

//! ---|> Evaluator
void BoxQualityEvaluator::endMeasure(FrameContext & /*context*/) {

	// calculate #triangles in as visible classified objects
	std::map<uintptr_t, GeometryNode*>::iterator iter;
	int32_t trianglesInAsVCObjects = 0;
	for (iter=objectsClassifiedAsV.begin(); iter!=objectsClassifiedAsV.end(); ++iter)
		trianglesInAsVCObjects += iter->second->getTriangleCount();

	// calculate #triangles in exactly visible objects
	int32_t trianglesInVObjects = 0;
	for (iter=objectsVisible.begin(); iter!=objectsVisible.end(); ++iter)
		trianglesInVObjects += iter->second->getTriangleCount();
	values->push_back(new Util::_NumberAttribute<int>(static_cast<int>(objectsClassifiedAsV.size())));
	values->push_back(new Util::_NumberAttribute<int>(static_cast<int>(objectsVisible.size())));
	values->push_back(new Util::_NumberAttribute<int>(trianglesInAsVCObjects));
	values->push_back(new Util::_NumberAttribute<int>(trianglesInVObjects));
}

}
}

#endif /* MINSG_EXT_EVALUATORS */
