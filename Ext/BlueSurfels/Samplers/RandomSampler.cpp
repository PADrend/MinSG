/*
	This file is part of the MinSG library extension BlueSurfels.
	Copyright (C) 2017-2018 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_BLUE_SURFELS

#include "RandomSampler.h"

#include <Rendering/Mesh/Mesh.h>

#include <Util/Timer.h>

#include <numeric>
#include <algorithm>

namespace MinSG {
namespace BlueSurfels {
  
Rendering::Mesh* RandomSampler::sampleSurfels(Rendering::Mesh* sourceMesh) {
  if(!sourceMesh) return nullptr;    
	Util::Timer t;
    
  uint32_t sampleCount = sourceMesh->getVertexCount();
  uint32_t surfelCount = std::min(getTargetCount(), sampleCount);
    
  std::vector<uint32_t> sampleIndices(sampleCount);
  std::iota(sampleIndices.begin(), sampleIndices.end(), 0);
  std::vector<uint32_t> surfelIndices(surfelCount);
  
  // extract random surfel indices
  std::uniform_int_distribution<uint32_t> random;
  typedef typename std::uniform_int_distribution<uint32_t>::param_type param_t;  
  uint32_t i=0;
  std::generate(surfelIndices.begin(), surfelIndices.end(), [&]() {
    std::swap(sampleIndices[i], sampleIndices[random(rng, param_t(i, static_cast<uint32_t>(sampleIndices.size()-1)))]);
    return sampleIndices[i++];
  });
  
	if(getStatisticsEnabled()) {
		statistics["t_sampling"] = static_cast<float>(t.getSeconds());
		statistics["num_samples"] = static_cast<float>(sampleCount);
		statistics["num_surfels"] = static_cast<float>(surfelCount);
	}
  
  return finalizeMesh(sourceMesh, surfelIndices);
}

} /* BlueSurfels */
} /* MinSG */
#endif // MINSG_EXT_BLUE_SURFELS