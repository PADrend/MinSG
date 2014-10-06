/*
	This file is part of the MinSG library extension BlueSurfels.
	Copyright (C) 2014 Claudius Jähn <claudius@uni-paderborn.de>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_BLUE_SURFELS

#ifndef SURFEL_ANALYSIS_H_
#define SURFEL_ANALYSIS_H_

#include <vector>
#include <cstdint>

namespace Rendering{
class Mesh;
}
namespace MinSG{
namespace BlueSurfels {

std::vector<float> getProgressiveMinimalMinimalVertexDistances(Rendering::Mesh& mesh);
std::vector<float> getMinimalVertexDistances(Rendering::Mesh& mesh,size_t prefixLength);
	
}
}

#endif // SURFEL_ANALYSIS_H_
#endif // MINSG_EXT_BLUE_SURFELS
