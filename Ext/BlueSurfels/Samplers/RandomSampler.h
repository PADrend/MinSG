/*
	This file is part of the MinSG library extension BlueSurfels.
	Copyright (C) 2017-2018 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_BLUE_SURFELS

#ifndef MINSG_EXT_RANDOM_SURFEL_GENERATOR_H_
#define MINSG_EXT_RANDOM_SURFEL_GENERATOR_H_

#include "AbstractSurfelSampler.h"

namespace MinSG {
namespace BlueSurfels {

class RandomSampler : public AbstractSurfelSampler {
	PROVIDES_TYPE_NAME(RandomSampler)
public:
	MINSGAPI virtual Rendering::Mesh* sampleSurfels(Rendering::Mesh* sourceMesh);
};

} /* BlueSurfels */
} /* MinSG */

#endif /* end of include guard: MINSG_EXT_RANDOM_SURFEL_GENERATOR_H_ */
#endif // MINSG_EXT_BLUE_SURFELS