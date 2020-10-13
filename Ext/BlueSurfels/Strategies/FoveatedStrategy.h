/*
	This file is part of the MinSG library extension BlueSurfels.
	Copyright (C) 2017-2018 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_BLUE_SURFELS

#ifndef BLUE_SURFELS_STRATEGIES_FOVEATEDSTRATEGY_H_
#define BLUE_SURFELS_STRATEGIES_FOVEATEDSTRATEGY_H_

#include "AbstractSurfelStrategy.h"

#include <Util/Macros.h>

#include <Geometry/Vec2.h>

#include <vector>

namespace MinSG {
namespace BlueSurfels {

class FoveatedStrategy : public AbstractSurfelStrategy {
	PROVIDES_TYPE_NAME(FoveatedStrategy)
	public:
		FoveatedStrategy() : AbstractSurfelStrategy(100), foveaZones({{0.25f, 2.0f}, {1.0f, 4.0f}}) {}
		MINSGAPI virtual bool update(MinSG::FrameContext& context, MinSG::Node* node, SurfelObject& surfel);
		
		GETSET(bool, Debug, false)
		GETSET(Geometry::Vec2, Offset, Geometry::Vec2(0,0))
		
		const std::vector<std::pair<float, float>>& getFoveaZones() const { return foveaZones; }
		void setFoveaZones(const std::vector<std::pair<float, float>>& zones) {	
			foveaZones.clear();
			foveaZones.assign(zones.begin(), zones.end()); 
			std::sort(foveaZones.begin(), foveaZones.end());	
		}
		void addFoveaZone(float pos, float scale) { foveaZones.emplace_back(pos, scale); std::sort(foveaZones.begin(), foveaZones.end());}
	private:
		std::vector<std::pair<float, float>> foveaZones;
		
};
  
} /* BlueSurfels */
} /* MinSG */

#endif /* end of include guard: BLUE_SURFELS_STRATEGIES_FOVEATEDSTRATEGY_H_ */
#endif // MINSG_EXT_BLUE_SURFELS