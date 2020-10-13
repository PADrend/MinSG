/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "CHCRenderer.h"
#include "OcclusionCullingStatistics.h"

#include "../../Core/Nodes/GeometryNode.h"
#include "../../Core/Nodes/GroupNode.h"
#include "../../Core/FrameContext.h"
#include "../../Core/NodeAttributeModifier.h"
#include "../../Core/Statistics.h"
#include "../../Core/Nodes/CameraNode.h"
#include "../../Helper/DistanceSorting.h"
#include "../../Helper/FrustumTest.h"
#include "../../Helper/StdNodeVisitors.h"

#include <Rendering/Draw.h>
#include <Rendering/OcclusionQuery.h>
#include <Util/Macros.h>
#include <Util/ObjectExtension.h>

#include <cassert>

using namespace Rendering;
using namespace Util;

namespace MinSG {

struct NodeInfo{
	bool visible, wasVisible;
	int frameNr;
	NodeInfo():visible(false),wasVisible(true),frameNr(0){}
};

static NodeInfo& getNodeInfo(Node& node){
	static const Util::StringIdentifier attrName( NodeAttributeModifier::create( "CHCOcclusionCullingInfo", NodeAttributeModifier::PRIVATE_ATTRIBUTE) );
	NodeInfo * i = Util::getObjectExtension<NodeInfo>(attrName,&node);
	if(!i)
		i = Util::addObjectExtension<NodeInfo>(attrName,&node);
	return *i;
}

// -----

//! ---|> [State]
State::stateResult_t CHCRenderer::doEnableState(FrameContext & context,Node * rootNode, const RenderParam & rp){
	if(rp.getFlag(SKIP_RENDERER))
		return State::STATE_SKIPPED;
	if(debugShowVisible){
		std::deque<Node *> nodes;
		{
			const auto children = getChildNodes(rootNode);
			nodes.insert(nodes.end(), children.begin(), children.end());
		}
		while(!nodes.empty()){
			Node& node = *nodes.front();
			nodes.pop_front();
			auto& nodeInfo = getNodeInfo(node);
			if(nodeInfo.frameNr == frameNr && nodeInfo.visible){
				if(node.isClosed())
					context.displayNode(&node,rp);
				else {
					const auto children = getChildNodes(&node);
					nodes.insert(nodes.end(), children.begin(), children.end());
				}
			}
		}
	}else{
		++frameNr;
		
		// used for statistics
		unsigned int stat_numTests = 0;
		unsigned int stat_numTestsInvisible = 0;
		unsigned int stat_numTestsVisible = 0;
		Statistics & statistics = context.getStatistics();

		RenderParam childParam = rp + USE_WORLD_MATRIX;

		const auto& camera = *context.getCamera();
		const Geometry::Vec3 camPos = camera.getWorldOrigin();
		const float bbEnlargement = camera.getNearPlane();
		NodeDistancePriorityQueue_F2B distanceQueue(camPos);
		std::queue<std::pair<Rendering::OcclusionQuery, Node*>> queryQueue;

		const auto traverseNode = [&](Node& node){
			if( node.isClosed() ){ // leaf node
				context.displayNode(&node, childParam );
			}else{
				for(auto & child : getChildNodes(&node))
					distanceQueue.push(child);
			}
		};
		const auto pullUpVisibility = [&](Node& node){
			for(Node* n=&node; n&&n!=rootNode; n = n->getParent()){
				auto& nodeInfo = getNodeInfo(*n);
				if(nodeInfo.frameNr == frameNr && nodeInfo.visible)
					break;
				nodeInfo.visible = true;
				nodeInfo.frameNr = frameNr;
				
			};
		};
		const auto startQuery = [&](Node& node,const Geometry::Box& worldBoundingBox){
			Rendering::OcclusionQuery query;
			Rendering::OcclusionQuery::enableTestMode(context.getRenderingContext());
			query.begin(context.getRenderingContext());
			Rendering::drawAbsBox(context.getRenderingContext(), worldBoundingBox );
			query.end(context.getRenderingContext());
			Rendering::OcclusionQuery::disableTestMode(context.getRenderingContext());
			return std::make_pair(std::move(query),&node);
		};
		
		traverseNode(*rootNode);
		while(!distanceQueue.empty() || !queryQueue.empty()){
				
			// ---- PART 1: process finished occlusion queries
			while( !queryQueue.empty() && (distanceQueue.empty() || queryQueue.front().first.isResultAvailable(context.getRenderingContext())) ){
				if( queryQueue.front().first.getResult(context.getRenderingContext())>0 ){
					++stat_numTestsVisible;
					statistics.pushEvent( Statistics::EVENT_TYPE_END_TEST_VISIBLE, 1);

					Node& node = *queryQueue.front().second;
					pullUpVisibility(node);
					auto& nodeInfo = getNodeInfo( node );
					if( !nodeInfo.wasVisible )
						traverseNode(node);
				}else{
					++stat_numTestsInvisible;
					statistics.pushEvent( Statistics::EVENT_TYPE_END_TEST_INVISIBLE, 1);
				}
				queryQueue.pop();
			}
			
			// ---- PART 2: tree traversal
			if( !distanceQueue.empty() ){
				Node& node = *distanceQueue.top();
				distanceQueue.pop();
				const Geometry::Box worldBoundingBox = node.getWorldBB();
				if (camera.testBoxFrustumIntersection( node.getWorldBB()) == Geometry::Frustum::intersection_t::OUTSIDE) {
					continue;
				}
				Geometry::Box enlargedBox = worldBoundingBox;
				enlargedBox.resizeAbs( bbEnlargement );
				if( enlargedBox.contains(camPos) ){
					pullUpVisibility(node);
					traverseNode(node);
					continue;
				}
				auto& nodeInfo = getNodeInfo( node );
				// identify previously visible nodes
				const bool wasVisible = nodeInfo.frameNr == frameNr-1 && nodeInfo.visible;
				nodeInfo.wasVisible = wasVisible;
				
				// reset node's visibility flag
				nodeInfo.visible = false;
				// update node's visited flag
				nodeInfo.frameNr = frameNr;
				
				// test leafs and previously invisible nodes
				if( node.isClosed() || !wasVisible){ // on testing front
					// start test
					++stat_numTests;
					statistics.pushEvent(Statistics::EVENT_TYPE_START_TEST, 1);
					queryQueue.push( std::move(startQuery(node,worldBoundingBox)) );
				}
				// traverse a node unless it was invisible
				if( wasVisible )
					traverseNode( node );
			}
		}

		statistics.addValue(OcclusionCullingStatistics::instance(statistics).getOccTestVisibleCounter(), stat_numTestsVisible);
		statistics.addValue(OcclusionCullingStatistics::instance(statistics).getOccTestInvisibleCounter(), stat_numTestsInvisible);
		statistics.addValue(OcclusionCullingStatistics::instance(statistics).getOccTestCounter(), stat_numTests);
	}
//			std::cout << std::endl;

	return State::STATE_SKIP_RENDERING;
}


}
