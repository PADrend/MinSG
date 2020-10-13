/*
	This file is part of the MinSG library extension BlueSurfels.
	Copyright (C) 2017-2018 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_BLUE_SURFELS

#ifndef BLUE_SURFELS_STRATEGIES_AdaptiveStrategy_H_
#define BLUE_SURFELS_STRATEGIES_AdaptiveStrategy_H_

#include "AbstractSurfelStrategy.h"

#include <Util/Macros.h>
#include <Util/Timer.h>

namespace MinSG {
namespace BlueSurfels {

class AdaptiveStrategy : public AbstractSurfelStrategy {
	PROVIDES_TYPE_NAME(AdaptiveStrategy)
	public:
		AdaptiveStrategy() : AbstractSurfelStrategy(10000) {}
		MINSGAPI virtual bool prepare(MinSG::FrameContext& context, MinSG::Node* node);
		MINSGAPI virtual bool update(MinSG::FrameContext& context, MinSG::Node* node, SurfelObject& surfel);
		MINSGAPI virtual void afterRendering(MinSG::FrameContext& context);
		GETSET(float, TargetTime, 16)
		GETSET(float, MaxSize, 8)
	private:
		Util::Timer timer;
		float size = 2;
};
  
} /* BlueSurfels */
} /* MinSG */

#endif /* end of include guard: BLUE_SURFELS_STRATEGIES_AdaptiveStrategy_H_ */
#endif // MINSG_EXT_BLUE_SURFELS