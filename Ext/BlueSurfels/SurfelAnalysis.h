/*
	This file is part of the MinSG library extension BlueSurfels.
	Copyright (C) 2014 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2016-2018 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_BLUE_SURFELS

#ifndef SURFEL_ANALYSIS_H_
#define SURFEL_ANALYSIS_H_

#include <vector>
#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <cmath>

namespace Rendering{
class Mesh;
}
namespace MinSG{
class FrameContext;
class Node;
namespace BlueSurfels {

std::vector<float> getProgressiveMinimalMinimalVertexDistances(Rendering::Mesh& mesh);
std::vector<float> getMinimalVertexDistances(Rendering::Mesh& mesh,size_t prefixLength);

float getMedianOfNthClosestNeighbours(Rendering::Mesh& mesh, size_t prefixLength, size_t nThNeighbour);

float getMeterPerPixel(MinSG::FrameContext & context, MinSG::Node * node);

inline uint32_t getPrefixForRadius(float radius, float medianDist, uint32_t medianCount, uint32_t maxCount) {
  return radius > 0 ? std::min<uint32_t>(medianCount * medianDist * medianDist / (radius * radius), maxCount) : 0;
}

inline float getRadiusForPrefix(uint32_t prefixLength, float medianDist, uint32_t medianCount) {
	return prefixLength > 0 ? medianDist * std::sqrt(static_cast<float>(medianCount) / static_cast<float>(prefixLength)) : 0;
}

inline float radiusToSize(float radius, float mpp) {
	return std::max(2.0f * radius / mpp, 1.0f);
}

inline float sizeToRadius(float size, float mpp) {
	return size * mpp * 0.5f;
}

}
}

#endif // SURFEL_ANALYSIS_H_
#endif // MINSG_EXT_BLUE_SURFELS
