/*
	This file is part of the MinSG library extension BlueSurfels.
	Copyright (C) 2017-2018 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_BLUE_SURFELS

#include "GreedyCluster.h"

#include <Geometry/Point.h>
#include <Geometry/PointOctree.h>

#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Mesh/MeshVertexData.h>
#include <Rendering/Mesh/VertexAttributeAccessors.h>
#include <Rendering/Mesh/VertexAttributeIds.h>

#include <Util/Timer.h>
#include <Util/UpdatableHeap.h>

#include <numeric>
#include <algorithm>
#include <iostream>
#include <queue>

namespace MinSG {
namespace BlueSurfels {

typedef std::pair<float,uint32_t> DistIndexPair_t;
	
struct Sample {
	float distance;
	uint32_t index;
	Geometry::Vec3 pos;
	
	inline
	bool operator<(const Sample& s) const {
		return distance < s.distance;
	}
};

typedef Util::UpdatableHeap<float, Sample> Heap_t;
typedef Heap_t::UpdatableHeapElement* HeapHandle_t;
	
static std::uniform_int_distribution<uint32_t> random;
typedef typename std::uniform_int_distribution<uint32_t>::param_type param_t;

// TODO: use friend lists instead of octree
// TODO: faster heap?
// TODO: parallel partition

Rendering::Mesh* GreedyCluster::sampleSurfels(Rendering::Mesh* sourceMesh) {
	if(!sourceMesh || sourceMesh->getVertexCount() == 0) return nullptr;
	Util::Timer t;
		
	struct OctreeEntry : public Geometry::Point<Geometry::Vec3f> {
		uint32_t surfelIndex;
		uint32_t index;
		OctreeEntry(uint32_t s, uint32_t i, const Geometry::Vec3 & p) : Geometry::Point<Geometry::Vec3f>(p), surfelIndex(s), index(i) {}
	};
	auto bb = sourceMesh->getBoundingBox();  
	Geometry::PointOctree<OctreeEntry> octree(bb,bb.getExtentMax()*0.01,16);
	auto acc = Rendering::PositionAttributeAccessor::create(sourceMesh->openVertexData(), Rendering::VertexAttributeIds::POSITION);
	
	uint32_t sampleCount = sourceMesh->getVertexCount();
	uint32_t targetCount = std::min(getTargetCount(), sampleCount);
	std::vector<std::vector<Sample>> servedSamples(targetCount);
	std::vector<HeapHandle_t> farthestElements(targetCount);
	std::vector<uint32_t> result;
	result.reserve(targetCount);
		
	Heap_t farthestHeap;
	uint32_t remainingSamples = sampleCount-1;
		
	{
		// initial surfel
		uint32_t index = random(rng, param_t(0,sampleCount-1));
		result.emplace_back(index);
		auto pos = acc->getPosition(index);
		octree.insert({0, index, pos});
			
		// initialize distances
		uint32_t i=0;
		Sample maxElt{0, index};
		servedSamples[0].resize(remainingSamples);
		for(auto& s : servedSamples[0]) {
			if(i == index) {
				++i;
				continue;
			}
			s.pos = acc->getPosition(i);
			s.distance = s.pos.distanceSquared(pos);
			s.index = i++;
			maxElt = std::max(maxElt, s);
		}
		
		farthestElements[0] = farthestHeap.insert(-maxElt.distance, maxElt);
	}
	
	if(getStatisticsEnabled()) {
		statistics["t_init"] = t.getSeconds();
		t.reset();
		sampleTimes.clear();
		sampleTimes.reserve(targetCount/1000);
	}
		
	Util::Timer heapTimer;
	double heapTime = 0;
	Util::Timer octreeTimer;
	double octreeTime = 0;
	Util::Timer updateTimer;
	double updateTime = 0;
		
	std::deque<OctreeEntry> friends;
	
	while(result.size() < targetCount && remainingSamples > 0) {
		auto q = farthestHeap.top();
		
		float radius = std::sqrt(q->data.distance);
		uint32_t index = q->data.index;
		uint32_t surfelIndex = result.size();
		auto pos = q->data.pos;
		
		if(radius < minRadius)
			break;
		
		octreeTimer.reset();
		friends.clear();
		octree.collectPointsWithinSphere({pos, radius*2.0f}, friends);
		octreeTime += octreeTimer.getSeconds();
		
		Sample maxQElt{0, 0};
		for(auto& f : friends) {
			Sample maxPElt{0, 0};
			updateTimer.reset();
			
			auto it = std::partition(servedSamples[f.surfelIndex].begin(), servedSamples[f.surfelIndex].end(), [&](Sample& s) {
				float dist = s.pos.distanceSquared(pos);
				if(dist < s.distance) {
					s.distance = dist;
					maxQElt = std::max(maxQElt, s);
					return false;
				} else {
					maxPElt = std::max(maxPElt, s);
					return true;
				}
			});
			servedSamples[surfelIndex].insert(servedSamples[surfelIndex].end(), it, servedSamples[f.surfelIndex].end());
			servedSamples[f.surfelIndex].erase(it, servedSamples[f.surfelIndex].end());
			
			updateTime += updateTimer.getSeconds();
			
			heapTimer.reset();
			farthestElements[f.surfelIndex]->data = maxPElt;
			farthestHeap.update(farthestElements[f.surfelIndex], -maxPElt.distance);
			heapTime += heapTimer.getSeconds();
		}
		heapTimer.reset();
		farthestElements[surfelIndex] = farthestHeap.insert(-maxQElt.distance, maxQElt);
		heapTime += heapTimer.getSeconds();
		
		octreeTimer.reset();
		octree.insert({surfelIndex, index, pos});
		octreeTime += octreeTimer.getSeconds();
		result.emplace_back(index);
		--remainingSamples;
	
		if(getStatisticsEnabled() && result.size() % 1000 == 0) {
			sampleTimes.emplace_back(t.getMilliseconds());
		}
	}
	
	if(getStatisticsEnabled()) {
		statistics["t_heap"] = heapTime;
		statistics["t_octree"] = octreeTime;
		statistics["t_partition"] = updateTime;
		statistics["t_sample"] = t.getSeconds();
		statistics["num_samples"] = sampleCount;
		statistics["num_surfels"] = result.size();
	}
	
	return finalizeMesh(sourceMesh, result);
}

} /* BlueSurfels */
} /* MinSG */
#endif // MINSG_EXT_BLUE_SURFELS