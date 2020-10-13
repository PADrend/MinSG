/*
	This file is part of the MinSG library extension BlueSurfels.
	Copyright (C) 2017-2018 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_BLUE_SURFELS

#ifndef MINSG_EXT_BLUESURFELS_SURFEL_SAMPLER_H_
#define MINSG_EXT_BLUESURFELS_SURFEL_SAMPLER_H_

#include <Util/ReferenceCounter.h>
#include <Util/TypeNameMacro.h>

#include <unordered_map>
#include <random>

namespace Rendering {
class Mesh;
}

namespace MinSG {
namespace BlueSurfels {

class AbstractSurfelSampler : public Util::ReferenceCounter<AbstractSurfelSampler> {
		PROVIDES_TYPE_NAME(AbstractSurfelSampler)
	public:
		MINSGAPI AbstractSurfelSampler();
		virtual ~AbstractSurfelSampler() = default;
		
		virtual Rendering::Mesh* sampleSurfels(Rendering::Mesh* sourceMesh) = 0;
		MINSGAPI static Rendering::Mesh* finalizeMesh(Rendering::Mesh* source, const std::vector<uint32_t>& indices);
				
		const std::unordered_map<std::string, float>& getStatistics() const { return statistics; }
		void clearStatistics() const { statistics.clear(); }
		bool getStatisticsEnabled() const { return statisticsEnabled; }
		void setStatisticsEnabled(bool v) { statisticsEnabled = v; }
		
		uint32_t getSeed() const { return seed; }
		void setSeed(uint32_t value) {
			seed = value;
			rng.seed(seed);
		}
			
		uint32_t getTargetCount() const { return targetCount; }
		void setTargetCount(uint32_t v) { targetCount = v; }
	protected:
		mutable std::unordered_map<std::string,float> statistics;
		mutable std::default_random_engine rng;
	private:
		uint32_t seed;
		uint32_t targetCount = 10000;
		bool statisticsEnabled = true;
};

} /* BlueSurfels */
} /* MinSG */

#endif /* end of include guard: MINSG_EXT_BLUESURFELS_SURFEL_SAMPLER_H_ */
#endif // MINSG_EXT_BLUE_SURFELS