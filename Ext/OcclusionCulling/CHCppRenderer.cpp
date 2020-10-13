/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "CHCppRenderer.h"
#include "OcclusionCullingStatistics.h"

#include "../../Core/Nodes/GeometryNode.h"
#include "../../Core/Nodes/GroupNode.h"
#include "../../Core/FrameContext.h"
#include "../../Core/NodeAttributeModifier.h"
#include "../../Core/Statistics.h"
#include "../../Core/Nodes/CameraNode.h"
#include "../../Helper/FrustumTest.h"
#include "../../Helper/StdNodeVisitors.h"

#include <Rendering/Draw.h>
#include <Rendering/OcclusionQuery.h>
#include <Util/Macros.h>
#include <Util/ObjectExtension.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <random>
#include <sstream>

namespace MinSG {
	
	struct CHCppRenderer::CHCppContext {
	public:
		CHCppContext(Util::StringIdentifier _nodeInfoId, uint32_t _frameNumber = 1) : nodeInfoId(std::move(_nodeInfoId)), frameNumber(_frameNumber) {}
        std::deque<Node*> invisibleQueue;
        std::queue<Node*> visibleQueue;
        std::queue<queue_item_t> queryQueue;
        Util::StringIdentifier nodeInfoId;
        uint32_t frameNumber;
		CHCppContext(const CHCppContext & other) = delete;
    };
	
	struct CHCppRenderer::NodeInfo {
    public:
        NodeInfo() : lastVisible(0u), skippedFramesTillQuery(0), pKeep(0.0f) {}
        uint32_t lastVisible;
        int32_t skippedFramesTillQuery;
        float pKeep;
    };

//! [ctor]
CHCppRenderer::CHCppRenderer(const unsigned int _visibilityThreshold /* = 0u */,
							 const unsigned int _maxPrevInvisNodesBatchSize /* = 50u */,
							 const unsigned int _skippedFramesTillQuery /* = 5u */,
							 const unsigned int _maxDepthForTightBoundingVolumes /* = 3u */,
							 const float _maxAreaDerivationForTightBoundingVolumes /* = 1.4f */)
	   : State(), mode(MODE_CULLING),
		visibilityThreshold(_visibilityThreshold),
		maxPrevInvisNodesBatchSize(_maxPrevInvisNodesBatchSize),
		skippedFramesTillQuery(_skippedFramesTillQuery),
		maxDepthForTightBoundingVolumes(_maxDepthForTightBoundingVolumes),
		maxAreaDerivationForTightBoundingVolumes(_maxAreaDerivationForTightBoundingVolumes)
{
	std::stringstream ss;
	ss << "CHCppContext_" << this;
	contextId = Util::StringIdentifier(  NodeAttributeModifier::create(ss.str(), NodeAttributeModifier::PRIVATE_ATTRIBUTE) ); // don't save, don't add to clones or instances
}

//! [dtor]
CHCppRenderer::~CHCppRenderer() = default;

CHCppRenderer::NodeInfo * CHCppRenderer::getNodeInfo(Node * node, CHCppContext & chcppContext) const{
	assert(node != nullptr);
	NodeInfo * i = Util::getObjectExtension<NodeInfo>(chcppContext.nodeInfoId,node);
	if(i==nullptr)
		i = Util::addObjectExtension<NodeInfo>(chcppContext.nodeInfoId,node);
	

	return i;
}

//! ---|> [Node]
CHCppRenderer * CHCppRenderer::clone() const
{
	return new CHCppRenderer(visibilityThreshold,
							 maxPrevInvisNodesBatchSize,
							 skippedFramesTillQuery,
							 maxDepthForTightBoundingVolumes,
							 maxAreaDerivationForTightBoundingVolumes);
}

//! ---|> [State]
State::stateResult_t CHCppRenderer::doEnableState(FrameContext & frameContext, Node * rootNode, const RenderParam & rp)
{
	if(!dynamic_cast<GroupNode*>(rootNode)){
		// doEnableState is only called for active states
		// this check should only be expensive in case of failure, right?
		// enable of chc++ on leafnodes makes no sense because the only effect would be that the leaf gets invisible because chc++ displays only children
		// if (some time) states are informed about attaching to nodes, this check should be moved to that method
		WARN("trying to enable a CHC++ on a non group node, deactivating self");
		deactivate();
		return State::STATE_SKIPPED;
	}
		
	CHCppContext* ctxt = Util::getObjectExtension<CHCppContext>(contextId,rootNode);
	if(!ctxt){
		std::stringstream ss;
		ss << "_CHCppNodeInfo_" << this << "@" << rootNode << "_";
		ctxt = Util::addObjectExtension<CHCppContext>(contextId,rootNode, NodeAttributeModifier::create(ss.str(), NodeAttributeModifier::PRIVATE_ATTRIBUTE) );
	}
	
	switch(mode)
	{
		case MODE_UNCONDITIONED:
			ctxt->frameNumber += skippedFramesTillQuery; // destroy temporal coherence and continue with MODE_CULLING
		case MODE_CULLING:
			return performCulling(frameContext,rootNode,rp,*ctxt);
		case MODE_SHOW_VISIBLE:
			return showVisible(frameContext,rootNode,rp,*ctxt);
		case MODE_SHOW_CULLED:
			return showCulled(frameContext,rootNode,rp,*ctxt);
		default:
			throw std::logic_error("CHC++: invalid rendermode");
	}
}

//! (internal) Main culling method
State::stateResult_t CHCppRenderer::performCulling(FrameContext & frameContext, Node * rootNode, const RenderParam & rp, CHCppContext & chcppContext)
{
	NodeDistancePriorityQueue_F2B distanceQueue(frameContext.getCamera()->getWorldOrigin());

	chcppContext.frameNumber++;

	resetStatistics();

	RenderParam childParam = rp + USE_WORLD_MATRIX;

	const auto children = getChildNodes(rootNode);
	for(auto & child : children) {
		distanceQueue.push(child);
	}

	// main loop
	while( (!distanceQueue.empty()) || (!chcppContext.queryQueue.empty()) )
	{
		while(!chcppContext.queryQueue.empty())
		{
			// if queries finished, handle them
			if(chcppContext.queryQueue.front().first.isResultAvailable(frameContext.getRenderingContext()))
			{
				handleReturnedQuery(frameContext, chcppContext.queryQueue.front(), childParam, distanceQueue, chcppContext);
				chcppContext.queryQueue.pop();
			// fill spare time by issuing query for the next previously visible node
			} else if(!chcppContext.visibleQueue.empty())
			{
				Rendering::OcclusionQuery::enableTestMode(frameContext.getRenderingContext());
				issueQuery(frameContext, chcppContext.visibleQueue.front(),chcppContext);
				chcppContext.visibleQueue.pop();
				Rendering::OcclusionQuery::disableTestMode(frameContext.getRenderingContext());
			}
		}

		// traverse scene
		if(!distanceQueue.empty())
		{
//			Node * node = *distanceQueue.rbegin();
//			distanceQueue.erase( --distanceQueue.end());
			Node * node = distanceQueue.top();
			distanceQueue.pop();
			NodeInfo * nodeInfo = getNodeInfo(node, chcppContext);

			// if node is inside view frustum
			if(frameContext.getCamera()->testBoxFrustumIntersection( node->getWorldBB()) != Geometry::Frustum::intersection_t::OUTSIDE)
			{
				if(!handleCameraInBox(frameContext, node, nodeInfo, childParam, distanceQueue,chcppContext))
				{
					// if node was invisible in last frame
					if(nodeInfo->lastVisible != (chcppContext.frameNumber - 1))
					{
						queryPreviouslyInvisibleNode(frameContext, node, chcppContext);
					} else {
						if(node->isClosed())
						{
							if(--nodeInfo->skippedFramesTillQuery < 0)
								chcppContext.visibleQueue.push(node);
							else
								pullUpVisibility(node, chcppContext);
						}

						traverseNode(frameContext, node, childParam, distanceQueue);
					}
				}
			}
		}

		if(distanceQueue.empty())
		{
			// issue remaining query batch
			issueMultiQueries(frameContext, chcppContext);
		}
	}

	// flush visibleQueue (remaining previously visible node queries)
	Rendering::OcclusionQuery::enableTestMode(frameContext.getRenderingContext());
	while(!chcppContext.visibleQueue.empty())
	{
		issueQuery(frameContext, chcppContext.visibleQueue.front(),chcppContext);
		chcppContext.visibleQueue.pop();
	}
	Rendering::OcclusionQuery::disableTestMode(frameContext.getRenderingContext());

	updateStatistics(frameContext);

	return State::STATE_SKIP_RENDERING;
}

//! (internal)
void CHCppRenderer::traverseNode(FrameContext & frameContext, Node * node, const RenderParam & rp, NodeDistancePriorityQueue_F2B & distanceQueue)
{
	if(!node->isActive())
		return;

	if(node->isClosed())
	{
		frameContext.displayNode(node, rp);
	} else {
		const auto children = getChildNodes(node);
		for(auto & child : children) {
			distanceQueue.push(child);
		}
	}
}

//! (internal)
void CHCppRenderer::pullUpVisibility(Node * node, CHCppContext & chcppContext) const
{
	NodeInfo * nodeInfo = getNodeInfo(node, chcppContext);

	while(nodeInfo->lastVisible != chcppContext.frameNumber)
	{
		nodeInfo->lastVisible = chcppContext.frameNumber;
		if(!node->hasParent())
			break;
		node = node->getParent();
		nodeInfo = getNodeInfo(node, chcppContext);
		
		// FIXME bis zum root? und wenn der renderer an einem teilbaum hängt?
	}
}

//! (internal)
void CHCppRenderer::handleReturnedQuery(FrameContext & frameContext, const queue_item_t & queryItem, const RenderParam & childParam, NodeDistancePriorityQueue_F2B & distanceQueue, CHCppContext & chcppContext)
{
	Statistics & statistics = frameContext.getStatistics();

	tests++;

	const Rendering::OcclusionQuery & currentQuery = queryItem.first;
	unsigned int result = currentQuery.getResult(frameContext.getRenderingContext());

	// Node is visible?
	if(result > visibilityThreshold)
	{
		testsVisible++;


		statistics.pushEvent(Statistics::EVENT_TYPE_END_TEST_VISIBLE, 1);

		// handle MultiQueries
		if(queryItem.second.size() > 1)
		{
			queryIndividualNodes(frameContext, queryItem, chcppContext);
		} else {
			Node * node = queryItem.second.front();
			NodeInfo * nodeInfo = getNodeInfo(node, chcppContext);

			if(nodeInfo->lastVisible != (chcppContext.frameNumber - 1))
			{
				traverseNode(frameContext, node, childParam, distanceQueue);
				static std::default_random_engine engine;
				nodeInfo->skippedFramesTillQuery = std::uniform_int_distribution<int>(0, skippedFramesTillQuery)(engine);
			} else {
				nodeInfo->skippedFramesTillQuery = skippedFramesTillQuery;
			}
			pullUpVisibility(node, chcppContext);
		}
	} else {
		testsInvisible++;
		statistics.pushEvent(Statistics::EVENT_TYPE_END_TEST_INVISIBLE, 1);

		if(result > 0 && queryItem.second.size() == 1){
			// FIXME What happens if the queryItem containes multiple nodes?
//			Node * node = queryItem.nodes[0];
//			NodeInfo * nodeInfo = getNodeInfo(node);
//
//			if(nodeInfo->getLastVisible() < (frameNumber - 3))
				RenderParam newRP(childParam);
				newRP.setChannel(FrameContext::APPROXIMATION_CHANNEL);
				frameContext.displayNode(queryItem.second.front(),newRP);
		}


	}
}

//! (internal)
void CHCppRenderer::queryPreviouslyInvisibleNode(FrameContext & frameContext, Node * node, CHCppContext & chcppContext)
{
	// calculate pKeep for node
	NodeInfo * nodeInfo = getNodeInfo(node, chcppContext);
	nodeInfo->pKeep = 0.99 - 0.7 * exp(-static_cast<int>(chcppContext.frameNumber - nodeInfo->lastVisible));

	chcppContext.invisibleQueue.push_back(node);
	if(chcppContext.invisibleQueue.size() >= maxPrevInvisNodesBatchSize)
		issueMultiQueries(frameContext, chcppContext);
}

//! (internal)
void CHCppRenderer::issueMultiQueries(FrameContext & frameContext, CHCppContext & chcppContext)
{
	Statistics & statistics = frameContext.getStatistics();
	statistics.pushEvent(Statistics::EVENT_TYPE_START_TEST, 1);

	Rendering::OcclusionQuery::enableTestMode(frameContext.getRenderingContext());

	std::sort(	chcppContext.invisibleQueue.begin(), 
				chcppContext.invisibleQueue.end(), 
				[this,&chcppContext](Node * nodeA, Node * nodeB) {
					return getNodeInfo(nodeA, chcppContext)->pKeep < getNodeInfo(nodeB, chcppContext)->pKeep;
				}
	);

	while(!chcppContext.invisibleQueue.empty())
	{
		std::deque<Node *> multiQueryNodes;

		float pKeepProduct = 1.0f;

		float benefit = 0;
		float cost;

		float oldValue = std::numeric_limits<float>::lowest();
		float value;

		while(!chcppContext.invisibleQueue.empty())
		{
			pKeepProduct *= getNodeInfo(chcppContext.invisibleQueue.front(), chcppContext)->pKeep;

			benefit += 1.0f;
			cost = 1.0f + (1.0f - pKeepProduct) * benefit;

			value = benefit / cost;

			if(value < oldValue)
				break;

			multiQueryNodes.push_back(chcppContext.invisibleQueue.front());
			chcppContext.invisibleQueue.pop_front();

			oldValue = value;
		}

		queue_item_t item;
		item.second.assign(multiQueryNodes.cbegin(), multiQueryNodes.cend());

		assert(!multiQueryNodes.empty());

		item.first.begin(frameContext.getRenderingContext());
		for(const auto & node : item.second) {
			drawTightBoundingVolume(frameContext, node);
		}
		item.first.end(frameContext.getRenderingContext());

		chcppContext.queryQueue.emplace(std::move(item));
	}

	Rendering::OcclusionQuery::disableTestMode(frameContext.getRenderingContext());
}

//! (internal)
void CHCppRenderer::queryIndividualNodes(FrameContext & frameContext, const queue_item_t & item, CHCppContext & chcppContext)
{
	Rendering::OcclusionQuery::enableTestMode(frameContext.getRenderingContext());
	for(const auto & node : item.second) {
		issueQuery(frameContext, node, chcppContext);
	}
	Rendering::OcclusionQuery::disableTestMode(frameContext.getRenderingContext());
}

//! (internal)
void CHCppRenderer::issueQuery(FrameContext & frameContext, Node * node, CHCppContext & chcppContext)
{
	Statistics & statistics = frameContext.getStatistics();
	statistics.pushEvent(Statistics::EVENT_TYPE_START_TEST, 1);

	queue_item_t item;
	item.second.push_back(node);

	item.first.begin(frameContext.getRenderingContext());
	drawTightBoundingVolume(frameContext, node);
	item.first.end(frameContext.getRenderingContext());

	chcppContext.queryQueue.emplace(std::move(item));
}

bool CHCppRenderer::handleCameraInBox(FrameContext & frameContext, Node * node, NodeInfo * nodeInfo, const RenderParam & childParam, NodeDistancePriorityQueue_F2B & distanceQueue, CHCppContext & chcppContext)
{
	Geometry::Box extendedBox = node->getWorldBB();
	extendedBox.resizeAbs(frameContext.getCamera()->getNearPlane());
	if(extendedBox.contains(frameContext.getCamera()->getWorldOrigin()))
	{
		static std::default_random_engine engine;
		nodeInfo->skippedFramesTillQuery = std::uniform_int_distribution<int>(0, skippedFramesTillQuery)(engine);

		traverseNode(frameContext ,node, childParam, distanceQueue);
		pullUpVisibility(node, chcppContext);
		return true;
	}
	return false;
}

void CHCppRenderer::drawTightBoundingVolume(FrameContext & frameContext, Node * node) const
{
	std::deque<Node *> nodes;
	std::deque<Node *> tempNodes;
	nodes.push_front(node);

	std::deque<Geometry::Box> boundingBoxes;
	std::deque<Geometry::Box> tempBoundingBoxes;
	boundingBoxes.push_front(node->getWorldBB());

	float nodeBoxSurfaceArea = boundingBoxes.front().getSurfaceArea();
	bool terminate = false;

	for(unsigned int i = 0; i < maxDepthForTightBoundingVolumes; i++)
	{
		unsigned int noChildrenCount = 0;

		for(auto & n : nodes)
		{
			const auto children = getChildNodes(n);
			if(children.empty()) {
				tempNodes.push_front(n);
				noChildrenCount++;
			} else {
				for(auto & child : children) {
					tempNodes.push_front(child);
				}
			}
		}

		if(noChildrenCount == nodes.size())
			break;

		float tightBoundingBoxSurfaceArea = 0;

		for(const auto & tempNode : tempNodes)
		{
			tempBoundingBoxes.push_front(tempNode->getWorldBB());

			tightBoundingBoxSurfaceArea += tempBoundingBoxes.front().getSurfaceArea();

			if(tightBoundingBoxSurfaceArea / nodeBoxSurfaceArea > maxAreaDerivationForTightBoundingVolumes)
			{
				terminate = true;
				break;
			}
		}

		if(terminate)
			break;

		nodes.swap(tempNodes);
		boundingBoxes.swap(tempBoundingBoxes);
	}

	for(auto & boundingBox : boundingBoxes){
//		Rendering::drawAbsBox(rc.getRenderingContext(), *itBoundingBoxes);
//		Rendering::drawFastAbsBox(rc.getRenderingContext(), *itBoundingBoxes);
		Rendering::drawFastAbsBox(frameContext.getRenderingContext(), boundingBox);

	}
//		Rendering::drawAbsBox(rc.getRenderingContext(), *itBoundingBoxes);
}

void CHCppRenderer::resetStatistics()
{
	tests					= 0;
	testsVisible			= 0;
	testsInvisible			= 0;
	occludedGeometryNodes	= 0;
}

void CHCppRenderer::updateStatistics(FrameContext & frameContext) {
	Statistics & statistics = frameContext.getStatistics();
	statistics.addValue(OcclusionCullingStatistics::instance(statistics).getOccTestCounter(), tests);
	statistics.addValue(OcclusionCullingStatistics::instance(statistics).getOccTestVisibleCounter(), testsVisible);
	statistics.addValue(OcclusionCullingStatistics::instance(statistics).getOccTestInvisibleCounter(), testsInvisible);
	statistics.addValue(OcclusionCullingStatistics::instance(statistics).getCulledGeometryNodeCounter(), occludedGeometryNodes);
}

State::stateResult_t CHCppRenderer::showVisible(FrameContext & frameContext,Node * rootNode, const RenderParam & rp, CHCppContext & chcppContext) const
{
	std::deque<Node *> nodes;
	nodes.push_back(rootNode);
	while(!nodes.empty())
	{
		Node * node = nodes.front();
		nodes.pop_front();
		NodeInfo * nInfo = getNodeInfo(node, chcppContext);
		if(nInfo->lastVisible != chcppContext.frameNumber)
			continue;
		else if(!conditionalFrustumTest(frameContext.getCamera()->getFrustum(), node->getWorldBB(), rp))
			continue;
		else if(node->isClosed())
			frameContext.displayNode(node, rp);
		else {
			const auto children = getChildNodes(node);
			nodes.insert(nodes.end(), children.begin(), children.end());
		}
	}
	return State::STATE_SKIP_RENDERING;
}

State::stateResult_t CHCppRenderer::showCulled(FrameContext & frameContext,Node * rootNode, const RenderParam & rp, CHCppContext & chcppContext) const
{
	std::deque<Node *> nodes;
	nodes.push_back(rootNode);
	while(!nodes.empty())
	{
		Node * node = nodes.front();
		nodes.pop_front();
		NodeInfo * nInfo = getNodeInfo(node, chcppContext);

		if(!conditionalFrustumTest(frameContext.getCamera()->getFrustum(), node->getWorldBB(), rp))
			continue;
		else if(node->isClosed() && nInfo->lastVisible != chcppContext.frameNumber)
			frameContext.displayNode(node, rp);
		else {
			const auto children = getChildNodes(node);
			nodes.insert(nodes.end(), children.begin(), children.end());
		}
	}
	return State::STATE_SKIP_RENDERING;
}

}
