/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "OccRenderer.h"
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

//! [ctor]
OccRenderer::OccRenderer():State(),frameNumber(0),mode(MODE_CULLING){
	//ctor
}

//! [dtor]
OccRenderer::~OccRenderer() {
	//dtor
}

OccRenderer::NodeInfo * OccRenderer::getNodeInfo(Node * node)const{
	assert(node!=nullptr);
	static const Util::StringIdentifier attrName( NodeAttributeModifier::create( "OcclusionCullingInfo", NodeAttributeModifier::PRIVATE_ATTRIBUTE) );

	NodeInfo * i=dynamic_cast<NodeInfo *>(node->getAttribute(attrName));
	if(i==nullptr){
		i=new NodeInfo();
		node->setAttribute(attrName,i);
	}
	return i;
}

void OccRenderer::updateNodeInformation(FrameContext & context,Node * rootNode)const{
	/***
	 ** Vis ---|> NodeVisitor
	 ***/
	struct Vis:public NodeVisitor {
		const OccRenderer & r;
		AbstractCameraNode * camera;
		const Node * rootNode;
		int insideFrustum;
		Geometry::Vec3 camPos;

		Vis(const OccRenderer & _r, const Node * _rootNode, AbstractCameraNode * _camera) :
				r(_r), camera(_camera), rootNode(_rootNode), insideFrustum(0), camPos(_camera->getWorldOrigin()) {}
		virtual ~Vis() {}

		// ---|> NodeVisitor
		NodeVisitor::status enter(Node * node) override {
			NodeInfo * info=r.getNodeInfo(node);
			info->setActualSubtreeComplexity(0);

			// the current subtree is completely inside the frustum, so we need not to test inside this subtree
			if(insideFrustum>0){
				info->setActualFrustumStatus(Geometry::Frustum::INSIDE);
				insideFrustum++;
			}else{
				int i=camera->testBoxFrustumIntersection( node->getWorldBB());
				info->setActualFrustumStatus(i);
				if(i==Geometry::Frustum::OUTSIDE){
					return NodeVisitor::BREAK_TRAVERSAL;
				}else if(i==Geometry::Frustum::INSIDE){
					insideFrustum++;
				}
			}
			if(node->isClosed() && node!=rootNode){
				unsigned int c = insideFrustum > 0 ? countTriangles(node) : countTrianglesInFrustum(node, camera->getFrustum());
				info->increaseActualSubtreeComplexity( c );
				return BREAK_TRAVERSAL;
			}
			return CONTINUE_TRAVERSAL;
		}
		// ---|> NodeVisitor
		NodeVisitor::status leave(Node * node) override {
			if(insideFrustum>0){
				insideFrustum--;
			}
			unsigned int complexity=r.getNodeInfo(node)->getActualSubtreeComplexity();
			if(complexity>0 && rootNode!=node && node->hasParent()){
				NodeInfo * pinfo=r.getNodeInfo(node->getParent());
				pinfo->increaseActualSubtreeComplexity(complexity);
			}
			return CONTINUE_TRAVERSAL;
		}
	} visitor(*this,rootNode,context.getCamera());
	rootNode->traverse(visitor);
	return;
}

//! ---|> [Node]
OccRenderer * OccRenderer::clone()const {
	return new OccRenderer();
}

//! ---|> [State]
State::stateResult_t OccRenderer::doEnableState(FrameContext & context,Node * rootNode, const RenderParam & rp){

	if (rp.getFlag(SKIP_RENDERER))
		return State::STATE_SKIPPED;

	switch(mode){
		 case MODE_CULLING:
		 case MODE_OPT_CULLING:
		 case MODE_UNCONDITIONED:
			return performCulling(context,rootNode,rp);
		//case MODE_SHOW_TESTS:
		case MODE_SHOW_VISIBLE:
			return showVisible(context,rootNode,rp);
		case MODE_SHOW_CULLED:
			return showCulled(context,rootNode,rp);
		default:
			std::cout << "\r Render Mode TODO!   ";
	}
	return State::STATE_OK;
}

//! (internal) Main culling method
State::stateResult_t OccRenderer::performCulling(FrameContext & context,Node * rootNode, const RenderParam & rp){
	if(mode==MODE_UNCONDITIONED){ // destroy temporal coherence
		frameNumber++;
	}
	frameNumber++;

	updateNodeInformation(context,rootNode); // ???

	// used for statistics
	unsigned int tests=0;
	int testsVisible=0;
	int testsInvisible=0;
	int occludedGeometryNodes=0;


	Statistics & statistics = context.getStatistics();

	RenderParam childParam = rp + USE_WORLD_MATRIX;

	const Geometry::Vec3 camPos=context.getCamera()->getWorldOrigin();


//	NodeDistancePriorityQueue_F2B distanceQueue(camPos);
	NodeDistancePriorityQueue_F2B distanceQueue(camPos);

	{	// add root node's children
		const auto children = getChildNodes(rootNode);
		for(auto & child : children) {
			distanceQueue.push(child);
		}
	}

	std::queue<std::pair<Rendering::OcclusionQuery, Node *>> queryQueue;

//    bool wasIdle=true;
	int idleCount=0;
	while ( (!distanceQueue.empty()) || (!queryQueue.empty() ) ) {
		bool idle=true;

		while ( !queryQueue.empty() && queryQueue.front().first.isResultAvailable() ) {
			idle=false;

			const OcclusionQuery & currentQuery = queryQueue.front().first;
			unsigned int result = currentQuery.getResult();
			Node * node = queryQueue.front().second;

			queryQueue.pop();
			NodeInfo * nodeInfo = getNodeInfo(node);

			// Node is visible?
			if (result>0 ) {

				statistics.pushEvent(Statistics::EVENT_TYPE_END_TEST_VISIBLE,nodeInfo->getActualSubtreeComplexity());

				testsVisible++;
				if (node->isClosed()) {
					// mark parents as visible
					Node * n=node;
					NodeInfo * ni=nodeInfo;
					while( ni->getVisibleFrameNumber()!=frameNumber){
						ni->setVisibleFrameNumber(frameNumber);
						n=n->getParent();
						if(n==nullptr || n==rootNode) break;
						ni=getNodeInfo(n);
					}
				}
				nodeInfo->setVisibleFrameNumber(frameNumber);

				// haben wir schon bearbeitet? -> Weiter...
				if (nodeInfo->getProcessedFrameNumber()==frameNumber)
					continue;

////                if(node->visibleFrameNumber != (frameNumber-1)){ ????
				// Process node
				processNode(context,node,nodeInfo,distanceQueue,childParam);

////                }
			}// Node is fully occluded?
			else {
				testsInvisible++;


				statistics.pushEvent(Statistics::EVENT_TYPE_END_TEST_INVISIBLE, nodeInfo->getActualSubtreeComplexity());

				// not processed yet?
				if (nodeInfo->getProcessedFrameNumber()!=frameNumber) {
					// TODO: Store and propagate in tree.
					occludedGeometryNodes+=collectNodesInFrustum<GeometryNode>(node,context.getCamera()->getFrustum()).size();

					// mark the node as processed
					nodeInfo->setProcessedFrameNumber(frameNumber);
				}
				// In diesem Frame verdeckt worden? (dann haben wir nicht mit der Bearbeitung gewartet und
				// die Kinder liegen bereits in der distanceQueue; die sollten jetzt �bersprungen werden)
				if ( nodeInfo->getVisibleFrameNumber() == (frameNumber-1) ) {
					if (!node->isClosed()) {
						const auto children = getChildNodes(node);
						for(auto & child : children) {
							getNodeInfo(child)->setProcessedFrameNumber(frameNumber);
							// TODO: Check if ancestors have to be removed too.
						}
					}
				}
			}
		}
		// ...
		if ( ! distanceQueue.empty()) {

			idle=false;

			Node * node=distanceQueue.top();
			NodeInfo * nodeInfo=getNodeInfo(node);
			distanceQueue.pop();

			// If the current node has already been processed, then continue.
			if (nodeInfo->getProcessedFrameNumber()==frameNumber ) continue; // ||node->isEmpty()

//			// skip empty subtrees // does never happen (checked in processNode)
//			if( nodeInfo->getActualSubtreeComplexity() == 0){
//                nullBoxes++;
//                continue;
//			}

			// Kamera innerhalb der (um die nearplane vergroesserte) Box? -> immer sichtbar
			// [2008-01-04_CJ] Bugfix: added nearplane addition; could result in dissapearing scene-parts.
			Geometry::Box extendedBox=node->getWorldBB(); //node->getGeometryBB();
			extendedBox.resizeAbs(context.getCamera()->getNearPlane());
			if (extendedBox.contains(camPos)) {

				nodeInfo->setVisibleFrameNumber(frameNumber);

				// Process node
				processNode(context,node,nodeInfo,distanceQueue,childParam);
				continue;
			}

			// !IS_INSIDE_FRUSTUM(node)
			if ( nodeInfo->getActualFrustumStatus()== Geometry::Frustum::OUTSIDE)
				continue;

			// [MODE_OPT_CULLING] For optimal culling, skip all unnecessary tests
			if(mode==MODE_OPT_CULLING && nodeInfo->getVisibleFrameNumber() == (frameNumber-1)){
				nodeInfo->setVisibleFrameNumber(frameNumber);
				// Process node
				processNode(context,node,nodeInfo,distanceQueue,childParam);
				continue;
			}

			// if query reasonable
			// TODO: Something missing here?

			{ // start occlusion test for the current node
				Rendering::OcclusionQuery query;

				Rendering::OcclusionQuery::enableTestMode(context.getRenderingContext());
				query.begin();
				Rendering::drawFastAbsBox(context.getRenderingContext(), node->getWorldBB());
				query.end();
				Rendering::OcclusionQuery::disableTestMode(context.getRenderingContext());
				tests++;
				queryQueue.emplace(std::move(query), node);

				statistics.pushEvent(Statistics::EVENT_TYPE_START_TEST, nodeInfo->getActualSubtreeComplexity());
			}

			// if the current node was visible in the last frame,
			//  then assume that it is also visible in this frame and process it.
			if (nodeInfo->getVisibleFrameNumber() == (frameNumber-1) ) {
				// Process node
				processNode(context,node,nodeInfo,distanceQueue,childParam);
			}

		}
//		else{
			// waiting

//		}
		if (idle) {
			statistics.pushEvent(Statistics::EVENT_TYPE_IDLE, idleCount++);
		} else {
			idleCount = 0;
		}

	}

	statistics.addValue(OcclusionCullingStatistics::instance(statistics).getOccTestCounter(), tests);
	statistics.addValue(OcclusionCullingStatistics::instance(statistics).getOccTestVisibleCounter(), testsVisible);
	statistics.addValue(OcclusionCullingStatistics::instance(statistics).getOccTestInvisibleCounter(), testsInvisible);
	statistics.addValue(OcclusionCullingStatistics::instance(statistics).getCulledGeometryNodeCounter(), occludedGeometryNodes);
	statistics.pushEvent(Statistics::EVENT_TYPE_FRAME_END,0.3);

	return State::STATE_SKIP_RENDERING;
}
/**
 * (internal)
 */
void OccRenderer::processNode(FrameContext & context,Node * node,NodeInfo * nodeInfo,NodeDistancePriorityQueue_F2B & distanceQueue,const RenderParam & rp){
	if (!node->isActive())
		return;

	nodeInfo->setProcessedFrameNumber(frameNumber);

	if(node->isClosed()){
		context.displayNode(node, rp );
	}else{
		const auto children = getChildNodes(node);
		for(auto & child : children){
			NodeInfo * i=getNodeInfo(child);
			if( rp.getFlag(FRUSTUM_CULLING) && i->getActualFrustumStatus()==Geometry::Frustum::OUTSIDE) //context.getCamera()->testBoxFrustumIntersection( (*it)->getWorldBB()) == Frustum::OUTSIDE )
				continue;
			if( i->getActualSubtreeComplexity() == 0){
				continue;
			}

			distanceQueue.push(child);
		}
	}
}

State::stateResult_t OccRenderer::showVisible(FrameContext & context,Node * rootNode, const RenderParam & rp){
	std::deque<Node *> nodes;
	nodes.push_back(rootNode);
	while(!nodes.empty()){
		Node * node=nodes.front();
		nodes.pop_front();
		NodeInfo * nInfo=getNodeInfo(node);
		if(nInfo->getVisibleFrameNumber()!=frameNumber)
			continue;
		else if(!conditionalFrustumTest(context.getCamera()->getFrustum(), node->getWorldBB(), rp))
			continue;
		else if(node->isClosed())
			context.displayNode(node,rp);
		else {
			const auto children = getChildNodes(node);
			nodes.insert(nodes.end(), children.begin(), children.end());
		}
	}
	return State::STATE_SKIP_RENDERING;
}

State::stateResult_t OccRenderer::showCulled(FrameContext & context,Node * rootNode, const RenderParam & rp){
	std::deque<Node *> nodes;
	nodes.push_back(rootNode);
	while(!nodes.empty()){
		Node * node=nodes.front();
		nodes.pop_front();
		NodeInfo * nInfo=getNodeInfo(node);

		if(!conditionalFrustumTest(context.getCamera()->getFrustum(), node->getWorldBB(), rp))
			continue;
		else if(node->isClosed() ){
			if(nInfo->getVisibleFrameNumber()!=frameNumber)
				context.displayNode(node,rp);
		}else {
			const auto children = getChildNodes(node);
			nodes.insert(nodes.end(), children.begin(), children.end());
		}
	}
	return State::STATE_SKIP_RENDERING;
}

}
