/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "NaiveOccRenderer.h"
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

#include <cassert>

using namespace Rendering;
using namespace Util;

namespace MinSG {

class NodeInfo : public Util::GenericAttribute{
	PROVIDES_TYPE_NAME(OccRenderer_NodeInfo)
		bool visible;
	public:
		NodeInfo():visible(false){}
		virtual ~NodeInfo(){}
		
		// ---|> GenericAttribute
		NodeInfo * clone()const override{
			NodeInfo * i = new NodeInfo;
			i->visible = visible;
			return i;
		}
		bool isVisible()const									{	return visible;	}
		void setVisible(bool b)									{	visible = b;	}
};

static NodeInfo * getNodeInfo(Node& node){
	static const Util::StringIdentifier attrName( NodeAttributeModifier::create( "NaiveOcclusionCullingInfo", NodeAttributeModifier::PRIVATE_ATTRIBUTE) );

	NodeInfo * i = dynamic_cast<NodeInfo *>(node.getAttribute(attrName));
	if(!i){
		i = new NodeInfo;
		node.setAttribute(attrName,i);
	}
	return i;
}

static bool getVisibilityStatus(Node& node){
	return getNodeInfo(node)->isVisible();
}
static void setVisibilityStatus(Node& node, bool b){
	return getNodeInfo(node)->setVisible(b);	
}

// -----

//! ---|> [State]
State::stateResult_t NaiveOccRenderer::doEnableState(FrameContext & context,Node * rootNode, const RenderParam & rp){
	if(rp.getFlag(SKIP_RENDERER))
		return State::STATE_SKIPPED;
	if(debugShowVisible){
		std::deque<Node *> nodes;
		nodes.push_back(rootNode);
		while(!nodes.empty()){
			Node * node = nodes.front();
			nodes.pop_front();
			if(getVisibilityStatus(*node)){
				if(node->isClosed())
					context.displayNode(node,rp);
				else {
					const auto children = getChildNodes(node);
					nodes.insert(nodes.end(), children.begin(), children.end());
				}
			}
		}
	}else{
		// used for statistics
		unsigned int stat_numTests = 0;
		unsigned int stat_numTestsInvisible = 0;
		unsigned int stat_numTestsVisible = 0;
		Statistics & statistics = context.getStatistics();

		RenderParam childParam = rp + USE_WORLD_MATRIX;

		const auto& camera = *context.getCamera();
		const Geometry::Vec3 camPos = camera.getWorldOrigin();
		const float bbEnlargement = camera.getNearPlane();
		Rendering::OcclusionQuery boxQuery;
		
		NodeDistancePriorityQueue_F2B distanceQueue(camPos);
		distanceQueue.push(rootNode);
		while(!distanceQueue.empty()){
			Node& node = *distanceQueue.top();
			distanceQueue.pop();
			
			bool nodeIsVisible = false;
			
			const Geometry::Box worldBoundingBox = node.getWorldBB();
			if (camera.testBoxFrustumIntersection(node.getWorldBB()) == Geometry::Frustum::intersection_t::OUTSIDE) {
				continue;
			}
			
			Geometry::Box enlargedBox = worldBoundingBox;
			enlargedBox.resizeAbs( bbEnlargement );
			if( enlargedBox.contains(camPos) ){
				nodeIsVisible = true;
			}else{
				++stat_numTests;
				Rendering::OcclusionQuery::enableTestMode(context.getRenderingContext());

				statistics.pushEvent(Statistics::EVENT_TYPE_START_TEST, 1);
				boxQuery.begin(context.getRenderingContext());
				Rendering::drawAbsBox(context.getRenderingContext(), worldBoundingBox );
				boxQuery.end(context.getRenderingContext());
				Rendering::OcclusionQuery::disableTestMode(context.getRenderingContext());
				const bool result = boxQuery.getResult(context.getRenderingContext());
				nodeIsVisible = result;
				if(result){ // statistics
					++stat_numTestsVisible;
					statistics.pushEvent( Statistics::EVENT_TYPE_END_TEST_VISIBLE, 1);
				}else{
					++stat_numTestsInvisible;
					statistics.pushEvent( Statistics::EVENT_TYPE_END_TEST_INVISIBLE, 1);
				}
			}
			setVisibilityStatus( node, nodeIsVisible );
			if( nodeIsVisible ){
				if( node.isClosed() ){ // leaf node
					context.displayNode(&node, childParam );
				}else{
					for(auto & child : getChildNodes(&node))
						distanceQueue.push(child);
				}
			}
		}

		statistics.addValue(OcclusionCullingStatistics::instance(statistics).getOccTestVisibleCounter(), stat_numTestsVisible);
		statistics.addValue(OcclusionCullingStatistics::instance(statistics).getOccTestInvisibleCounter(), stat_numTestsInvisible);
		statistics.addValue(OcclusionCullingStatistics::instance(statistics).getOccTestCounter(), stat_numTests);
	}
	return State::STATE_SKIP_RENDERING;
}


}
