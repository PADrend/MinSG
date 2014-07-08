/*
	This file is part of the MinSG library extension BlueSurfels.
	Copyright (C) 2014 Claudius Jähn <claudius@uni-paderborn.de>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_BLUE_SURFELS

#ifndef SURFEL_RENDERER
#define SURFEL_RENDERER

#include "../../Core/States/NodeRendererState.h"

#include <Geometry/Vec3.h>

namespace MinSG{

namespace BlueSurfels {
	
class SurfelRenderer : public NodeRendererState{
	PROVIDES_TYPE_NAME(SurfelRenderer)
	public:
		SurfelRenderer();
		SurfelRenderer(const SurfelRenderer&) = default;
		virtual ~SurfelRenderer();
		
		NodeRendererResult displayNode(FrameContext & context, Node * node, const RenderParam & rp) override;
		stateResult_t doEnableState(FrameContext & context, Node * node, const RenderParam & rp) override;

		float getCountFactor()const		{	return countFactor;	}
		float getMaxSideLength()const	{	return maxSideLength;	}
		float getMinSideLength()const	{	return minSideLength;	}
		float getSizeFactor()const		{	return sizeFactor;	}

		void setCountFactor(float f)	{	countFactor = f;	}
		void setMaxSideLength(float f)	{	maxSideLength = f;	}
		void setMinSideLength(float f)	{	minSideLength = f;	}
		void setSizeFactor(float f)		{	sizeFactor = f;	}
		
		SurfelRenderer* clone()const	{	return new SurfelRenderer(*this);	}
	private:
		float minSideLength,maxSideLength,countFactor,sizeFactor;
		float projectionScale;
		Geometry::Vec3 cameraOrigin;
};
}

}

#endif // SURFEL_RENDERER
#endif // MINSG_EXT_BLUE_SURFELS
