/*
	This file is part of the MinSG library extension BlueSurfels.
	Copyright (C) 2014 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2016-2018 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_BLUE_SURFELS

#ifndef MINSG_EXT_BLUESURFELS_SURFEL_ANALYSIS_H_
#define MINSG_EXT_BLUESURFELS_SURFEL_ANALYSIS_H_

#define M_2SQRT3 1.7320508075688772935274463415059
#define DEFAULT_PACKING_QUALITY 0.74

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
class AbstractCameraNode;
namespace BlueSurfels {

std::vector<float> getProgressiveMinimalMinimalVertexDistances(Rendering::Mesh& mesh);
std::vector<float> getMinimalVertexDistances(Rendering::Mesh& mesh,size_t prefixLength);

float getMedianOfNthClosestNeighbours(Rendering::Mesh& mesh, size_t prefixLength, size_t nThNeighbour);

float getMeterPerPixel(AbstractCameraNode* camera, MinSG::Node* node);

float computeSurfelPacking(Rendering::Mesh* mesh);

float getSurfelPacking(MinSG::Node* node, Rendering::Mesh* surfels);

Rendering::Mesh* getSurfels(MinSG::Node * node);

inline uint32_t getPrefixForRadius(float radius, float packing) {
	return radius > 0 ? packing/(radius*radius) : 0;
}

inline float getRadiusForPrefix(uint32_t prefix, float packing) {
	return prefix > 0 ? std::sqrt(packing / static_cast<float>(prefix)) : 0;
}

inline float radiusToSize(float radius, float mpp) {
	return std::max(2.0f * radius / mpp, 1.0f);
}

inline float sizeToRadius(float size, float mpp) {
	return size * mpp * 0.5f;
}

}
}

#endif // MINSG_EXT_BLUESURFELS_SURFEL_ANALYSIS_H_
#endif // MINSG_EXT_BLUE_SURFELS
