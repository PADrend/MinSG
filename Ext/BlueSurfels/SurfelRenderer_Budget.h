/*
	This file is part of the MinSG library extension BlueSurfels.
	Copyright (C) 2017 Sascha Brandt <myeti@mail.uni-paderborn.de>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_BLUE_SURFELS

#ifndef SURFEL_RENDERER_Budget_H_
#define SURFEL_RENDERER_Budget_H_

#include "../../Core/States/NodeRendererState.h"
#include "../../Core/Nodes/CameraNode.h"

#include <Geometry/Vec3.h>

namespace Rendering {
class Mesh;
} 

namespace MinSG{

namespace BlueSurfels {
	
class SurfelRendererBudget : public NodeRendererState{
	PROVIDES_TYPE_NAME(SurfelRendererBudget)
	public:
		SurfelRendererBudget();
		SurfelRendererBudget(const SurfelRendererBudget&) = default;
		virtual ~SurfelRendererBudget();
		
		NodeRendererResult displayNode(FrameContext & context, Node * node, const RenderParam & rp) override;

		float getCountFactor()const		{	return countFactor;	}
		float getSizeFactor()const		{	return sizeFactor;	}
		float getMaxSurfelSize()const		{	return maxSurfelSize;	}
		bool getDebugHideSurfels() const { return debugHideSurfels; }
		bool isDebugCameraEnabled() const { return debugCameraEnabled; }

		void setCountFactor(float f)	{	countFactor = f;	}
		void setSizeFactor(float f)		{	sizeFactor = f;	}
		void setMaxSurfelSize(float f)		{	maxSurfelSize = f;	}
		void setDebugHideSufels(bool b) { debugHideSurfels = b; }
		void setDebugCameraEnabled(bool b);
		
		SurfelRendererBudget* clone()const	{	return new SurfelRendererBudget(*this);	}
	private:
		float countFactor,sizeFactor,maxSurfelSize;
		bool debugHideSurfels, debugCameraEnabled;
		Util::Reference<CameraNode> debugCamera;
		
		float getMedianDist(Node * node, Rendering::Mesh& mesh);
		double getBudget(Node* node);
};
}

}

#endif // SURFEL_RENDERER_Budget_H_
#endif // MINSG_EXT_BLUE_SURFELS
