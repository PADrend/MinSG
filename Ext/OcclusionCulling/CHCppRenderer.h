/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef CHCPPRENDERER_H
#define CHCPPRENDERER_H

#include "../../Core/States/State.h"
#include "../../Helper/DistanceSorting.h"
#include <Rendering/OcclusionQuery.h>
#include <Util/GenericAttribute.h>
#include <queue>

namespace MinSG {
class Statistics;
/**
 *  @see Mattausch O., Bittner J., Wimmer M.: CHC++: Coherent Hierarchical Culling Revisited.
 *  http://www.cg.tuwien.ac.at/research/publications/2008/mattausch-2008-CHC/
 *
 *  [CHCppRenderer] ---|> [State]
 * @ingroup states
 */
class CHCppRenderer : public State
{
    PROVIDES_TYPE_NAME(CHCppRenderer)

private:

    struct CHCppContext;

    struct NodeInfo;

public:
	
    enum renderMode
    {
        MODE_CULLING		= 0,
        MODE_SHOW_VISIBLE	= 1,
        MODE_SHOW_CULLED	= 2,
        MODE_UNCONDITIONED	= 3
    };

    MINSGAPI explicit CHCppRenderer(const unsigned int _visibilityThreshold = 0u,
                           const unsigned int _maxPrevInvisNodesBatchSize = 50u,
                           const unsigned int _skippedFramesTillQuery = 5u,
                           const unsigned int _maxDepthForTightBoundingVolumes = 3u,
                           const float _maxAreaDerivationForTightBoundingVolumes = 1.4f);
    MINSGAPI virtual ~CHCppRenderer();

    MINSGAPI NodeInfo * getNodeInfo(Node * node, CHCppContext & chcppContext) const;

    renderMode getMode() const{ return mode;  }
    void setMode(const renderMode newMode){  mode = newMode;  }

    // ---|> [State]
    MINSGAPI CHCppRenderer * clone() const override;

    unsigned int getVisibilityThreshold() const						{ return visibilityThreshold; }
    void setVisibilityThreshold(const unsigned int i)				{ visibilityThreshold = i; }
    
    unsigned int getMaxPrevInvisNodesBatchSize() const				{ return maxPrevInvisNodesBatchSize; }
    void setMaxPrevInvisNodesBatchSize(const unsigned int i)		{ maxPrevInvisNodesBatchSize = i; }
    
    unsigned int getSkippedFramesTillQuery() const					{ return skippedFramesTillQuery; }
    void setSkippedFramesTillQuery(const unsigned int i)			{ skippedFramesTillQuery = i; }
    
    unsigned int getMaxDepthForTightBoundingVolumes() const			{ return maxDepthForTightBoundingVolumes; }
    void setMaxDepthForTightBoundingVolumes(const unsigned int i)	{ maxDepthForTightBoundingVolumes = i; }
    
    float getMaxAreaDerivationForTightBoundingVolumes() const		{ return maxAreaDerivationForTightBoundingVolumes; }
    void setMaxAreaDerivationForTightBoundingVolumes(const float f)	{ maxAreaDerivationForTightBoundingVolumes = f; }

private:
	
    typedef std::pair<Rendering::OcclusionQuery, std::vector<Node *>> queue_item_t;

    renderMode mode;

    unsigned int visibilityThreshold;
    unsigned int maxPrevInvisNodesBatchSize;
    unsigned int skippedFramesTillQuery;
    unsigned int maxDepthForTightBoundingVolumes;
    float maxAreaDerivationForTightBoundingVolumes;

    Util::StringIdentifier contextId;

    // used for statistics
    int tests;
    int testsVisible;
    int testsInvisible;
    int occludedGeometryNodes; // not calculated

    MINSGAPI stateResult_t doEnableState(FrameContext & frameContext,Node *, const RenderParam & rp) override;
	
    MINSGAPI State::stateResult_t showVisible(FrameContext & frameContext, Node * rootNode, const RenderParam & rp, CHCppContext & chcppContext) const;
    MINSGAPI State::stateResult_t showCulled(FrameContext & frameContext, Node * rootNode, const RenderParam & rp, CHCppContext & chcppContext) const;
    MINSGAPI State::stateResult_t performCulling(FrameContext & frameContext, Node * rootNode, const RenderParam & rp, CHCppContext & chcppContext);
	
    MINSGAPI void traverseNode(FrameContext & frameContext, Node * node, const RenderParam & rp, NodeDistancePriorityQueue_F2B & distanceQueue);
    MINSGAPI void pullUpVisibility(Node * node, CHCppContext & chcppContext) const;
    MINSGAPI void handleReturnedQuery(FrameContext & frameContext, const queue_item_t & queryItem, const RenderParam & childRp, NodeDistancePriorityQueue_F2B & distanceQueue, CHCppContext & chcppContext);
    MINSGAPI void queryPreviouslyInvisibleNode(FrameContext & frameContext, Node * node, CHCppContext & chcppContext);
    MINSGAPI void issueMultiQueries(FrameContext & frameContext, CHCppContext & chcppContext);
    MINSGAPI void queryIndividualNodes(FrameContext & frameContext, const queue_item_t & item, CHCppContext & chcppContext);
    MINSGAPI void issueQuery(FrameContext & frameContext, Node * node, CHCppContext & chcppContext);
    MINSGAPI bool handleCameraInBox(FrameContext & frameContext, Node * node, NodeInfo * nodeInfo, const RenderParam & childRp, NodeDistancePriorityQueue_F2B & distanceQueue, CHCppContext & chcppContext);

    MINSGAPI void drawTightBoundingVolume(FrameContext & frameContext, Node * node) const;

    MINSGAPI void resetStatistics();
    MINSGAPI void updateStatistics(FrameContext & frameContext);


};
}

#endif // CHCPPRENDERER_H
