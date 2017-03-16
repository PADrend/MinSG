/*
	This file is part of the MinSG library extension BlueSurfels.
	Copyright (C) 2014 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2016-2017 Sascha Brandt <myeti@mail.uni-paderborn.de>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_BLUE_SURFELS

#ifndef SURFEL_RENDERER_FIXE_SIZE_H_
#define SURFEL_RENDERER_FIXE_SIZE_H_

#include "../../Core/States/NodeRendererState.h"
#include "../../Core/Nodes/CameraNode.h"

#include <Geometry/Vec3.h>

namespace Rendering {
class Mesh;
} 

namespace MinSG{

namespace BlueSurfels {
	
class SurfelRendererFixedSize : public NodeRendererState{
	PROVIDES_TYPE_NAME(SurfelRendererFixedSize)
	public:
		SurfelRendererFixedSize();
		SurfelRendererFixedSize(const SurfelRendererFixedSize&) = default;
		virtual ~SurfelRendererFixedSize();
		
		NodeRendererResult displayNode(FrameContext & context, Node * node, const RenderParam & rp) override;

		float getCountFactor()const		{	return countFactor;	}
		float getSizeFactor()const		{	return sizeFactor;	}
		float getMaxSurfelSize()const		{	return maxSurfelSize;	}
		bool getDebugHideSurfels() const { return debugHideSurfels; }
		bool isDebugCameraEnabled() const { return debugCameraEnabled; }
		bool getDeferredSurfels() const { return deferredSurfels; }

		void setCountFactor(float f)	{	countFactor = f;	}
		void setSizeFactor(float f)		{	sizeFactor = f;	}
		void setMaxSurfelSize(float f)		{	maxSurfelSize = f;	}
		void setDebugHideSufels(bool b) { debugHideSurfels = b; }
		void setDebugCameraEnabled(bool b) { debugCamera = nullptr; debugCameraEnabled = b; };
		void setDeferredSurfels(bool b) { deferredSurfels = b; }
		
		SurfelRendererFixedSize* clone()const	{	return new SurfelRendererFixedSize(*this);	}
		
		void drawSurfels(FrameContext & context) const;
	protected:
		stateResult_t doEnableState(FrameContext & context, Node * node, const RenderParam & rp) override;
		void doDisableState(FrameContext & context, Node * node, const RenderParam & rp) override;
	private:			
		float countFactor,sizeFactor,maxSurfelSize;
		bool debugHideSurfels, debugCameraEnabled, deferredSurfels;
		Util::Reference<CameraNode> debugCamera;
		std::deque<std::tuple<Node*,uint32_t,float,float>> deferredSurfelQueue;
				
		float getMedianDist(Node * node, Rendering::Mesh& mesh);
};
}

}

#endif // SURFEL_RENDERER_FIXE_SIZE_H_
#endif // MINSG_EXT_BLUE_SURFELS