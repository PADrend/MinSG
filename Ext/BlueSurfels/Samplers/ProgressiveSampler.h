/*
	This file is part of the MinSG library extension BlueSurfels.
	Copyright (C) 2014 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2016-2018 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_BLUE_SURFELS

#ifndef MINSG_EXT_BLUESURFELS_PROGRESSIVE_SAMPLER_H_
#define MINSG_EXT_BLUESURFELS_PROGRESSIVE_SAMPLER_H_

#include "AbstractSurfelSampler.h"

namespace MinSG {
namespace BlueSurfels {

class ProgressiveSampler : public AbstractSurfelSampler {
	PROVIDES_TYPE_NAME(ProgressiveSampler)
public:
    MINSGAPI virtual Rendering::Mesh* sampleSurfels(Rendering::Mesh* sourceMesh);	
	uint32_t getSamplesPerRound() const { return samplesPerRound; }
	void setSamplesPerRound(uint32_t v) { samplesPerRound = v; }
private:
	uint32_t samplesPerRound = 200;
};

} /* BlueSurfels */
} /* MinSG */

#endif /* end of include guard: MINSG_EXT_BLUESURFELS_PROGRESSIVE_SAMPLER_H_ */
#endif // MINSG_EXT_BLUE_SURFELS