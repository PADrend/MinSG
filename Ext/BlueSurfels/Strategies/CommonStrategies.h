/*
	This file is part of the MinSG library extension BlueSurfels.
	Copyright (C) 2017-2018 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_BLUE_SURFELS

#ifndef BLUE_SURFELS_STRATEGIES_FIXEDSIZE_H_
#define BLUE_SURFELS_STRATEGIES_FIXEDSIZE_H_

#include "AbstractSurfelStrategy.h"
#include "../SurfelAnalysis.h"

#include <Util/Macros.h>

namespace Rendering {
class Texture;
} /* Rendering */

namespace MinSG {
class AbstractCameraNode;
namespace BlueSurfels {
	
class FixedSizeStrategy : public AbstractSurfelStrategy {
	PROVIDES_TYPE_NAME(FixedSizeStrategy)
	public:
		FixedSizeStrategy() : AbstractSurfelStrategy(1000) {}		
		MINSGAPI virtual bool update(MinSG::FrameContext& context, MinSG::Node* node, SurfelObject& surfel);
		GETSET(float, Size, 2)
};

class FixedCountStrategy : public AbstractSurfelStrategy {
PROVIDES_TYPE_NAME(FixedCountStrategy)
public:
	FixedCountStrategy() : AbstractSurfelStrategy(900) {}
	MINSGAPI virtual bool update(MinSG::FrameContext& context, MinSG::Node* node, SurfelObject& surfel);
	GETSET(uint32_t, Count, 0)
};

class FactorStrategy : public AbstractSurfelStrategy {
PROVIDES_TYPE_NAME(FactorStrategy)
public:
	FactorStrategy() : AbstractSurfelStrategy(-1100) {}
	MINSGAPI virtual bool update(MinSG::FrameContext& context, MinSG::Node* node, SurfelObject& surfel);
	GETSET(float, CountFactor, 1)
	GETSET(float, SizeFactor, 1)
};

class BlendStrategy : public AbstractSurfelStrategy {
	PROVIDES_TYPE_NAME(BlendStrategy)
	public:
		BlendStrategy() : AbstractSurfelStrategy(-1000) {}		
		MINSGAPI virtual bool update(MinSG::FrameContext& context, MinSG::Node* node, SurfelObject& surfel);
		GETSET(float, Blend, 0.3f)
};
	
class ReferencePointStrategy : public AbstractSurfelStrategy {
	PROVIDES_TYPE_NAME(ReferencePointStrategy)
	public:
		ReferencePointStrategy() : AbstractSurfelStrategy(10000) {}		
		MINSGAPI virtual bool update(MinSG::FrameContext& context, MinSG::Node* node, SurfelObject& surfel);
		GETSET(ReferencePoint, ReferencePoint, ReferencePoint::CLOSEST_SURFEL)
};

class DebugStrategy : public AbstractSurfelStrategy {
	PROVIDES_TYPE_NAME(DebugStrategy)
	public:
		DebugStrategy();
		MINSGAPI virtual bool prepare(MinSG::FrameContext& context, MinSG::Node* node);
		MINSGAPI virtual bool update(MinSG::FrameContext& context, MinSG::Node* node, SurfelObject& surfel);
		MINSGAPI virtual bool beforeRendering(MinSG::FrameContext& context);
		MINSGAPI virtual void afterRendering(MinSG::FrameContext& context);
		inline bool getFixSurfels() const { return fixSurfels; }
		MINSGAPI void setFixSurfels(bool value);
		GETSET(bool, HideSurfels, false)
		GETSET(uint32_t, FixedSurfelCount, 0)
		GETSET(float, DebugColorScreen, 0)
		MINSGAPI void setHeatmap(Rendering::Texture* texture);
		MINSGAPI Rendering::Texture* getHeatmap() const;
	private:
		bool fixSurfels = false;
		Util::Reference<MinSG::AbstractCameraNode> debugCamera;
		Util::Reference<Rendering::Texture> heatmap;
};
  
} /* BlueSurfels */
} /* MinSG */

#endif /* end of include guard: BLUE_SURFELS_STRATEGIES_FIXEDSIZE_H_ */
#endif // MINSG_EXT_BLUE_SURFELS