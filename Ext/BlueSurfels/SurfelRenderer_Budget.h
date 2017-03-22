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
#include <set>

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
		void setDebugHideSurfels(bool b) { debugHideSurfels = b; }
		void setDebugCameraEnabled(bool b) { debugCamera = nullptr; debugCameraEnabled = b; }
		
		double getBudget() const { return budget; }
		void setBudget(double b) { budget = b; }
		
		bool getDeferredSurfels() const { return deferredSurfels; }
		void setDeferredSurfels(bool b) { deferredSurfels = b; }
		
		bool getDebugAssignment() const { return debugAssignment; }
		void setDebugAssignment(bool b) { debugAssignment = b; }
		
		float getMaxIncrement() const { return maxIncrement; }
		void setMaxIncrement(float value) { maxIncrement = value; }
		
		SurfelRendererBudget* clone()const	{	return new SurfelRendererBudget(*this);	}
		
		void drawSurfels(FrameContext & context, float minSize=0, float maxSize=1024) const;
		void assignmentStep() { stepAssignment = true; };
		void clearAssignment() { doClearAssignment = true; };
	protected:
		stateResult_t doEnableState(FrameContext & context, Node * node, const RenderParam & rp) override;
		void doDisableState(FrameContext & context, Node * node, const RenderParam & rp) override;
	private:	
		
		struct BudgetAssignment {
			// Runtime values; required for rendering
			Node* node = nullptr; // the associated node 
			uint32_t prefix = 0; // the current assigned prefix length
			float size = 1; // the current point size
			// surfel parameters
			float medianDist = 0; // median distance at 1000 points
			float mpp = 0; // meter per pixel
			uint32_t surfelCount = 0; // number of available surfels 
			uint32_t maxPrefix = 0; // max. prefix length (obj. is covered with size 1)
			uint32_t minPrefix = 0; // min. prefix length (obj. is covered with max. size)
			float minSize = 1;
			// expansion
			bool expanded = false;
			float expansionCost = 0;
			float expansionBenefit = 0;
			float expansionThreshold = 0;
			// other
			double surfelBenefit = 0;
			uint64_t frame = 0;
		};
		struct BudgetCmp {
			constexpr bool operator()(const BudgetAssignment* la, const BudgetAssignment* ra) const {
				return la->surfelBenefit > ra->surfelBenefit || (la->surfelBenefit == ra->surfelBenefit && la->node > ra->node);
			} 
		};
		
		double budget=1e+6, usedBudget=0, surfelBenefitSum=0;
		float maxSurfelSize=32, surfelCostFactor=1, geoCostFactor=1, benefitGrowRate=2, maxIncrement=1000;
		bool debugHideSurfels = false, debugCameraEnabled=false, deferredSurfels=true, debugAssignment=false, doClearAssignment=false, stepAssignment=false;
		double debugSurfelUsed = 0;
		double debugGeoUsed = 0;
		uint64_t frameCount = 0;
		
		Util::Reference<CameraNode> debugCamera;
		std::set<BudgetAssignment*, BudgetCmp> assignments;
		
		//double getSurfelBenefit(float x, float ps, float sat) const;
		//double getSurfelBenefitDerivative(float x, float ps, float sat) const;
		float getMedianDist(Node * node, Rendering::Mesh* mesh);
		BudgetAssignment& getAssignment(Node* node);
		void assignBudget();
};
}

}

#endif // SURFEL_RENDERER_Budget_H_
#endif // MINSG_EXT_BLUE_SURFELS
