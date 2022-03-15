/*
	This file is part of the MinSG library extension BlueSurfels.
	Copyright (C) 2014 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2016-2018 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_BLUE_SURFELS

#include "ProgressiveSampler.h"

#include <Geometry/Point.h>
#include <Geometry/PointOctree.h>

#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Mesh/MeshVertexData.h>
#include <Rendering/Mesh/VertexAttributeAccessors.h>
#include <Rendering/Mesh/VertexAttributeIds.h>

#include <Util/Timer.h>

#include <numeric>
#include <algorithm>
#include <iostream>

namespace MinSG {
namespace BlueSurfels {
  
typedef std::pair<float, uint32_t> DistSurfelPair_t;

static std::vector<uint32_t> extractRandom(std::default_random_engine& rng, std::vector<uint32_t>& source, uint32_t num) {
  num = std::min<uint32_t>(num, static_cast<uint32_t>(source.size()));
  static std::uniform_int_distribution<uint32_t> random;
  typedef typename std::uniform_int_distribution<uint32_t>::param_type param_t;
  std::vector<uint32_t> result(num);
  uint32_t i=static_cast<uint32_t>(source.size()-1);
  std::generate(result.begin(), result.end(), [&]() {
    std::swap(source[i], source[random(rng, param_t(0, i))]);
    return source[i--];
  });
  source.erase(source.begin() + source.size() - num, source.end());
  return result;
}

Rendering::Mesh* ProgressiveSampler::sampleSurfels(Rendering::Mesh* sourceMesh) {
  if(!sourceMesh || sourceMesh->getVertexCount() == 0) return nullptr;  
	Util::Timer t;
    
  uint32_t sampleCount = sourceMesh->getVertexCount();
  uint32_t targetCount = std::min(getTargetCount(), sampleCount);
  
  std::vector<uint32_t> sampleIndices(sampleCount);
  std::iota(sampleIndices.begin(), sampleIndices.end(), 0);
  std::vector<uint32_t> surfelIndices;
  surfelIndices.reserve(targetCount);
  
  struct OctreeEntry : public Geometry::Point<Geometry::Vec3f> {
  	uint32_t surfelId;
  	OctreeEntry(uint32_t i,const Geometry::Vec3 & p) : Geometry::Point<Geometry::Vec3f>(p), surfelId(i) {}
  };  
  auto bb = sourceMesh->getBoundingBox();
	Geometry::PointOctree<OctreeEntry> octree(bb,bb.getExtentMax()*0.01f,16);
  auto acc = Rendering::PositionAttributeAccessor::create(sourceMesh->openVertexData(), Rendering::VertexAttributeIds::POSITION);
    
  // initial surfel
  uint32_t surfelIndex = extractRandom(rng, sampleIndices, 1).front();
  octree.insert({surfelIndex, acc->getPosition(surfelIndex)});
  surfelIndices.emplace_back(surfelIndex);
  
  uint32_t accept = 0;
  uint32_t round = 0;
  uint32_t samplesPerRound = getSamplesPerRound();
	std::deque<OctreeEntry> closest;
	std::vector<DistSurfelPair_t> sortedSubset; // surfelId , distance to nearest other sample
  
	while(surfelIndices.size() < targetCount) {
    samplesPerRound = std::min<uint32_t>(samplesPerRound, static_cast<uint32_t>(sampleIndices.size()));
		const auto randomSubset = extractRandom(rng, sampleIndices, samplesPerRound);
		for(const auto& index : randomSubset) {
			const Geometry::Vec3& pos = acc->getPosition(index);
			closest.clear();
			octree.getClosestPoints(pos, 1, closest);
			sortedSubset.emplace_back(pos.distanceSquared(closest.front().getPosition()), index);
		}
		// highest quality at the back
		std::partial_sort(sortedSubset.rbegin(), sortedSubset.rbegin() + accept + 1, sortedSubset.rend(), std::greater<DistSurfelPair_t>());
    
		// accept best samples
		for(uint32_t j = 0; j < accept && !sortedSubset.empty() && surfelIndices.size() < targetCount; ++j) {
			auto& sample = sortedSubset.back();
			sortedSubset.pop_back();
			const uint32_t index = sample.second;
			octree.insert(OctreeEntry(index, acc->getPosition(index)));
      surfelIndices.emplace_back(index);
		}

		// remove one (bad) surfel to prevent using duplicate surfels even if the requested number of surfels is too large.
		if(!sortedSubset.empty())
			sortedSubset.pop_back();
      
		// put back unused samples
		for(const auto& entry : sortedSubset)
			sampleIndices.emplace_back(entry.second);
      
    sortedSubset.clear();
    if( (round%500) == 0){
      samplesPerRound = static_cast<uint32_t>(std::max(samplesPerRound * 0.5f, 20.0f));
      accept = std::min(static_cast<uint32_t>(samplesPerRound * 0.3), accept + 1);
    }
    ++round;
  }
  
	if(getStatisticsEnabled()) {
		statistics["t_sampling"] = static_cast<float>(t.getSeconds());
		statistics["num_samples"] = static_cast<float>(sampleCount);
		statistics["num_surfels"] = static_cast<float>(surfelIndices.size());
	}
  
  return finalizeMesh(sourceMesh, surfelIndices);
}

} /* BlueSurfels */
} /* MinSG */
#endif // MINSG_EXT_BLUE_SURFELS