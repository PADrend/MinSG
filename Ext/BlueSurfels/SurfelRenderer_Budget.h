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

#include <vector>
#include <unordered_map>

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

		float getMaxSurfelSize()const		{	return maxSurfelSize;	}
		bool getDebugHideSurfels() const { return debugHideSurfels; }
		bool isDebugCameraEnabled() const { return debugCameraEnabled; }

		void setMaxSurfelSize(float f)		{	maxSurfelSize = f;	}
		void setDebugHideSufels(bool b) { debugHideSurfels = b; }
		void setDebugCameraEnabled(bool b) { debugCamera = nullptr; debugCameraEnabled = b; }
		
		double getBudget() const { return budget; }
		void setBudget(double b) { budget = b; }
		
		bool getDeferredSurfels() const { return deferredSurfels; }
		void setDeferredSurfels(bool b) { deferredSurfels = b; }
		
		SurfelRendererBudget* clone()const	{	return new SurfelRendererBudget(*this);	}
		
		void drawSurfels(FrameContext & context) const;
	protected:
		stateResult_t doEnableState(FrameContext & context, Node * node, const RenderParam & rp) override;
		void doDisableState(FrameContext & context, Node * node, const RenderParam & rp) override;
	private:	
		struct SurfelAssignment {
			Node* node;
			uint32_t prefix;
			float radius;
			uint32_t maxPrefix;
			uint32_t minPrefix;
			float ps; // projected size
			float mpp; // meter per pixel
			float smd; // surfel median distance
		};
		
		float maxSurfelSize, surfelCostFactor, geoCostFactor, benefitGrowRate;
		bool debugHideSurfels, debugCameraEnabled, deferredSurfels;
		double budget, budgetRemainder, usedBudget;
		uint32_t maxTime;
		Util::Reference<CameraNode> debugCamera;
		std::vector<SurfelAssignment> surfelAssignments;
				
		double getSurfelBenefit(float x, float ps, float sat) const;
		double getSurfelBenefitDerivative(float x, float ps, float sat) const;
		float getMedianDist(Node * node, Rendering::Mesh* mesh);
};
}

}

#endif // SURFEL_RENDERER_Budget_H_
#endif // MINSG_EXT_BLUE_SURFELS
