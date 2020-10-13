/*
	This file is part of the MinSG library extension BlueSurfels.
	Copyright (C) 2017-2018 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_BLUE_SURFELS

#ifndef MINSG_EXT_BLUESURFELS_GreedyCluster_H_
#define MINSG_EXT_BLUESURFELS_GreedyCluster_H_

#include "AbstractSurfelSampler.h"

#include <map>

namespace MinSG {
namespace BlueSurfels {

/**
 * Computes an exact greedy permutation of the input samples.
 *
 * Inspired by:
 * Sariel Har-Peled and Manor Mendel
 * Fast Construction of Nets in Low-Dimensional Metrics and Their Applications
 * SIAM Journal on Computing (2006). 
 * https://doi.org/10.1137/s0097539704446281
 */
class GreedyCluster : public AbstractSurfelSampler {
	PROVIDES_TYPE_NAME(GreedyCluster)
public:
	MINSGAPI virtual Rendering::Mesh* sampleSurfels(Rendering::Mesh* sourceMesh);
	
	void setMinRadius(float r) { minRadius = r; }
	float getMinRadius() const { return minRadius; }
	
	std::map<uint32_t,float> getSampleTimes() const { return sampleTimes; }
private:
	float minRadius = 0;
	std::map<uint32_t,float> sampleTimes;
};

} /* BlueSurfels */
} /* MinSG */

#endif /* end of include guard: MINSG_EXT_BLUESURFELS_GreedyCluster_H_ */
#endif // MINSG_EXT_BLUE_SURFELS