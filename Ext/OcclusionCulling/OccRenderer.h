/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef OCCRENDERER_H
#define OCCRENDERER_H

#include "../../Core/States/State.h"
#include "../../Helper/DistanceSorting.h"
#include <Geometry/Frustum.h>
#include <Util/GenericAttribute.h>
#include <queue>

namespace MinSG {

/**
 *  [OccRenderer] ---|> [State]
 * @ingroup states
 */
class OccRenderer : public State {
		PROVIDES_TYPE_NAME(OccRenderer)
	public:
		enum renderMode {
			MODE_CULLING = 0,
			MODE_SHOW_VISIBLE = 1,
			MODE_SHOW_CULLED = 2,
			/*! Start only occlusion tests for invisible nodes.
				\note Only works if real culling has been performed at this position
			 */
			MODE_OPT_CULLING = 3,
			MODE_UNCONDITIONED = 4
		};
		class NodeInfo : public Util::GenericAttribute{
			PROVIDES_TYPE_NAME(OccRenderer_NodeInfo)
			private:
				unsigned int visibleFrameNumber;
				unsigned int processedFrameNumber;
				unsigned int actualSubtreeComplexity;
				Geometry::Frustum::intersection_t actualFrustumStatus;
			public:
				NodeInfo() : 
						visibleFrameNumber(0),
						processedFrameNumber(0),
						actualSubtreeComplexity(0),
						actualFrustumStatus(Geometry::Frustum::intersection_t::INSIDE) {
				}
				virtual ~NodeInfo(){}
				// ---|> GenericAttribute
				NodeInfo * clone()const override{
					NodeInfo * i=new NodeInfo();
					i->processedFrameNumber=processedFrameNumber;
					i->visibleFrameNumber=visibleFrameNumber;
					return i;
				}
				unsigned int getVisibleFrameNumber()const				{	return visibleFrameNumber;	}
				void setVisibleFrameNumber(unsigned int f)				{	visibleFrameNumber=f;	}
				unsigned int getProcessedFrameNumber()const				{	return processedFrameNumber;	}
				void setProcessedFrameNumber(unsigned int f)			{   processedFrameNumber=f;	}
				unsigned int getActualSubtreeComplexity()const			{	return actualSubtreeComplexity;	}
				void increaseActualSubtreeComplexity(unsigned int c)	{	actualSubtreeComplexity+=c;	}
				void setActualSubtreeComplexity(unsigned int f)			{	actualSubtreeComplexity=f;	}
				Geometry::Frustum::intersection_t getActualFrustumStatus() const {
					return actualFrustumStatus;
				}
				void setActualFrustumStatus(Geometry::Frustum::intersection_t status) {
					actualFrustumStatus = status;
				}
		};
		// -----

		MINSGAPI OccRenderer();
		MINSGAPI virtual ~OccRenderer();

		MINSGAPI NodeInfo * getNodeInfo(Node * node)const;

		renderMode getMode()const			{	return mode;	}
		void setMode(renderMode newMode)	{	mode=newMode;	}

		MINSGAPI OccRenderer * clone() const override;

	private:
		unsigned int frameNumber;
		renderMode mode;

		MINSGAPI State::stateResult_t performCulling(FrameContext & context,Node * rootNode, const RenderParam & rp);
		MINSGAPI void updateNodeInformation(FrameContext & context,Node * rootNode)const;
		MINSGAPI void processNode(FrameContext & context,Node * node,NodeInfo * nodeInfo,NodeDistancePriorityQueue_F2B & distanceQueue,const RenderParam & rp);

		MINSGAPI State::stateResult_t showVisible(FrameContext & context,Node * rootNode, const RenderParam & rp);
		MINSGAPI State::stateResult_t showCulled(FrameContext & context,Node * rootNode, const RenderParam & rp);

		MINSGAPI stateResult_t doEnableState(FrameContext & context, Node *, const RenderParam & rp) override;

};
}

#endif // OCCRENDERER_H
