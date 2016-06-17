/*
	This file is part of the MinSG library extension BlueSurfels.
	Copyright (C) 2014 Claudius J�hn <claudius@uni-paderborn.de>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_BLUE_SURFELS

#ifndef SURFEL_RENDERER2
#define SURFEL_RENDERER2

#include "../../Core/States/NodeRendererState.h"

#include <Geometry/Vec3.h>

namespace MinSG{

namespace BlueSurfels {
	
class SurfelRenderer2 : public NodeRendererState{
	PROVIDES_TYPE_NAME(SurfelRenderer2)
	public:
		SurfelRenderer2();
		SurfelRenderer2(const SurfelRenderer2&) = default;
		virtual ~SurfelRenderer2();
		
		NodeRendererResult displayNode(FrameContext & context, Node * node, const RenderParam & rp) override;

		float getCountFactor()const		{	return countFactor;	}
		float getSizeFactor()const		{	return sizeFactor;	}
		float getMaxSurfelSize()const		{	return maxSurfelSize;	}
		bool getDebugHideSurfels() const { return debugHideSurfels; }

		void setCountFactor(float f)	{	countFactor = f;	}
		void setSizeFactor(float f)		{	sizeFactor = f;	}
		void setMaxSurfelSize(float f)		{	maxSurfelSize = f;	}
		void setDebugHideSufels(bool b) { debugHideSurfels = b; }
		
		SurfelRenderer2* clone()const	{	return new SurfelRenderer2(*this);	}
	private:
		float countFactor,sizeFactor,maxSurfelSize;
		bool debugHideSurfels;
};
}

}

#endif // SURFEL_RENDERER2
#endif // MINSG_EXT_BLUE_SURFELS
