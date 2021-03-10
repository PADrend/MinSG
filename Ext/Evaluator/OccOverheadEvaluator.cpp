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

#include "OccOverheadEvaluator.h"
#include "../../Core/Nodes/GeometryNode.h"
#include "../../Core/Nodes/GroupNode.h"
#include "../../Core/Statistics.h"
#include "../../Core/FrameContext.h"
#include "../../Helper/StdNodeVisitors.h"
#include <Geometry/Box.h>
#include <Rendering/RenderingContext/RenderingParameters.h>
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Rendering/Draw.h>
#include <Rendering/OcclusionQuery.h>
#include <Util/Graphics/Color.h>
#include <Util/GenericAttribute.h>
#include <Util/Timer.h>
#include <algorithm>
#include <list>

namespace MinSG {
namespace Evaluators {

//! [ctor]
OccOverheadEvaluator::OccOverheadEvaluator(DirectionMode _mode/*=SINGLE_VALUE*/)
		:Evaluator(_mode), renderingTime(0.0f),renderingFlags(FRUSTUM_CULLING) {
	setMaxValue_f(20.0f);
}

//! [dtor]
OccOverheadEvaluator::~OccOverheadEvaluator() {
}

//! ---|> Evaluator
void OccOverheadEvaluator::beginMeasure() {
	renderingTime=0;
	values->clear();
}

//! ---|> Evaluator
void OccOverheadEvaluator::measure(FrameContext & context,Node & node,const Geometry::Rect & /*r*/) {

	//float currentRenderingTime=0;
	//int numTests=0;
	//int numVBO=0;
	int numPoly=0;
	Util::Color4f clearColor(0.5f, 0.5f, 0.9f, 1.0f);
	{ // 1. normal render pass
//        rc.clearScreen(0.5,0.5,0.9);
//        context.getRenderingContext().finish();
//        node.display(rc,renderingFlags);
		context.getRenderingContext().clearScreen(clearColor);
		node.display(context,renderingFlags|USE_WORLD_MATRIX);

		context.getRenderingContext().clearScreen(clearColor);
		context.getRenderingContext().finish();
		context.getStatistics().beginFrame();
		node.display(context,renderingFlags|USE_WORLD_MATRIX);
		context.getRenderingContext().finish();
		context.getStatistics().endFrame();
		numPoly=static_cast<int>(context.getStatistics().getValueAsDouble(context.getStatistics().getTrianglesCounter()));
	}

	// 2. test all group nodes if their bb is visible and collect them
	std::deque<GroupNode *> visibleGroupNodes;

	{
		// collect geometry nodes in frustum
		const auto groupNodesInVFList = collectNodesInFrustum<GroupNode>(&node,context.getCamera()->getFrustum());

		// setup occlusion queries
		size_t numQueries=groupNodesInVFList.size();
		if (numQueries==0) {
			values->push_back(nullptr); // TODO!!!
			return; // TODO??
		}

		auto queries = new Rendering::OcclusionQuery[numQueries];

		//  test bounding boxes
		context.getRenderingContext().pushAndSetDepthBuffer(Rendering::DepthBufferParameters(true, true, Rendering::Comparison::LEQUAL));
		int i=0;
		// start queries
		Rendering::OcclusionQuery::enableTestMode(context.getRenderingContext());
		for(const auto & groupNode : groupNodesInVFList) {
			queries[i].begin();
//            queries[i].fastBoxTest((*it)->getWorldBB());
			Rendering::drawAbsBox(context.getRenderingContext(), groupNode->getWorldBB() );
			queries[i].end();
			++i;
		}
		Rendering::OcclusionQuery::disableTestMode(context.getRenderingContext());
		i=0;
		// get results
		for(const auto & groupNode : groupNodesInVFList) {
			Geometry::Box extendedBox=groupNode->getWorldBB();
			extendedBox.resizeAbs(context.getCamera()->getNearPlane());

			if (queries[i].getResult() > 0 || extendedBox.contains(context.getCamera()->getWorldOrigin()) ) {
				visibleGroupNodes.push_back(groupNode);
			}
			++i;
		}
		context.getRenderingContext().popDepthBuffer();

		delete [] queries;
	}
	std::deque<GeometryNode *> geoNodesInVisibleGroupNodes;

	// 3. collect all direct geometry-node children
	{
		struct GeoChildrenCollector:public NodeVisitor {
			GeoChildrenCollector(std::deque<GeometryNode*> & _geoNodes,FrameContext & _context):
				geoNodes(_geoNodes),firstNode(true),frameContext(_context) {}
			virtual ~GeoChildrenCollector() {}

			std::deque<GeometryNode*> & geoNodes;
			bool firstNode;
			FrameContext & frameContext;

			// ---|> NodeVisitor
			NodeVisitor::status enter(Node * _node) override {
				// if this is the first call, continue...
				if(firstNode) {
					firstNode=false;
					return CONTINUE_TRAVERSAL;
				}
				// else collect the geometry children and break the traversal.
				GeometryNode * gn=dynamic_cast<GeometryNode *>(_node);
				if(gn){
					if(frameContext.getCamera()->testBoxFrustumIntersection( _node->getWorldBB()) != Geometry::Frustum::intersection_t::OUTSIDE)
						geoNodes.push_back(gn);
				}
				return BREAK_TRAVERSAL;
			}
		};
		for (auto & visibleGroupNode : visibleGroupNodes) {
			GeoChildrenCollector vis(geoNodesInVisibleGroupNodes,context);
			(visibleGroupNode)->traverse(vis);
		}

	}

	// 4. clear screen and measure time drawing children
	float perfectRenderingTime=0;
	{
		context.getRenderingContext().clearScreen(clearColor);

		context.getRenderingContext().finish();
		Util::Timer timer;
		timer.reset();

		for (auto & geoNodesInVisibleGroupNode : geoNodesInVisibleGroupNodes) {
			context.displayNode((geoNodesInVisibleGroupNode),renderingFlags|USE_WORLD_MATRIX);
		}

		context.getRenderingContext().finish();
		timer.stop();
		perfectRenderingTime=static_cast<float>(timer.getMilliseconds());
	}

//    // Test-costs:
//    float result=numTests!=0?(currentRenderingTime-perfectRenderingTime)/numTests:0;

	// currentRenderingTime-perfRendering:
//    float result=currentRenderingTime-perfectRenderingTime;

	float result=numPoly>0?perfectRenderingTime*1000/numPoly:0;

//    std::cout <<currentRenderingTime<<"-"<<perfectRenderingTime<<"="<<result<<"\t"<<numTests<<"\n";
//    std::cout <<numVBO<<":"<<geoNodesInVisibleGroupNodes.size()<<"\n";

//   float result=perfectRenderingTime;
//    values.push_back(currentRenderingTime);
//    values.push_back(perfectRenderingTime);
	values->push_back(new Util::_NumberAttribute<float>(result));

//    if(mode==SINGLE_VALUE){
//        if(result>renderingTime || renderingTime==0)
//            renderingTime=result;
//    }else{
//        values.push_back(result);
//    }

//    renderingTime=0;
}

//! ---|> Evaluator
void OccOverheadEvaluator::endMeasure(FrameContext & /*context*/) {
//    if(mode==SINGLE_VALUE)
//        values.push_back(renderingTime);
//    if (whiteShader) whiteShader->disable();
	//std::map

}

}
}

#endif /* MINSG_EXT_EVALUATORS */
