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

#include <vector>
#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <cmath>

#include <Util/References.h>

namespace Rendering{
class Mesh;
}
namespace Util {
class Bitmap;
} /* Util */
namespace MinSG{
class FrameContext;
class Node;
class AbstractCameraNode;
namespace BlueSurfels {
	
enum ReferencePoint : uint8_t {
	CLOSEST_BB,
	FARTHEST_BB,
	CENTER_BB,
	CLOSEST_SURFEL
};

struct Radial {
	float mean;
	float variance;
	uint32_t count;
};

MINSGAPI std::vector<float> getProgressiveMinimalMinimalVertexDistances(Rendering::Mesh& mesh);
MINSGAPI std::vector<float> getMinimalVertexDistances(Rendering::Mesh& mesh,size_t prefixLength, bool geodesic=false);

MINSGAPI float getMedianOfNthClosestNeighbours(Rendering::Mesh& mesh, size_t prefixLength, size_t nThNeighbour);

MINSGAPI float computeRelPixelSize(AbstractCameraNode* camera, MinSG::Node* node, ReferencePoint ref = ReferencePoint::CLOSEST_BB);

MINSGAPI float computeSurfelPacking(Rendering::Mesh* mesh);

MINSGAPI float getSurfelPacking(MinSG::Node* node, Rendering::Mesh* surfels);

MINSGAPI Rendering::Mesh* getSurfels(MinSG::Node * node);

//! Differential domain analysis based on "Differential domain analysis for non-uniform sampling" by Wei et al. (ACM ToG 2011)
MINSGAPI Util::Reference<Util::Bitmap> differentialDomainAnalysis(Rendering::Mesh* mesh, float diff_max, int32_t resolution=256, uint32_t count=0, bool geodesic=true, bool adaptive=false);
MINSGAPI std::vector<Radial> getRadialMeanVariance(const Util::Reference<Util::Bitmap>& spectrum);

inline uint32_t getPrefixForRadius(float radius, float packing) {
	return radius > 0 ? packing/(radius*radius) : 0;
}

inline float getRadiusForPrefix(uint32_t prefix, float packing) {
	return prefix > 0 ? std::sqrt(packing / static_cast<float>(prefix)) : 0;
}

inline float radiusToSize(float radius, float relPixelSize) {
	return std::max(2.0f * radius / relPixelSize, 1.0f);
}

inline float sizeToRadius(float size, float relPixelSize) {
	return size * relPixelSize * 0.5f;
}

}
}

#endif // MINSG_EXT_BLUESURFELS_SURFEL_ANALYSIS_H_
#endif // MINSG_EXT_BLUE_SURFELS
