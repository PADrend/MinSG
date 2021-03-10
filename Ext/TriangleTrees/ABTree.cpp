/*
	This file is part of the MinSG library extension TriangleTrees.
	Copyright (C) 2009-2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_TRIANGLETREES

#include "ABTree.h"
#include "TriangleAccessor.h"
#include <Geometry/Box.h>
#include <Geometry/Definitions.h>
#include <cmath>
#include <limits>
#include <vector>

namespace Rendering {
class Mesh;
}
namespace MinSG {
namespace TriangleTrees {

ABTree::ABTree(Rendering::Mesh * mesh,
				uint32_t trianglesPerNode, float allowedBBEnlargement) :
	kDTree(mesh, trianglesPerNode, allowedBBEnlargement), splitDimension(0) {
}

ABTree::ABTree(const Geometry::Box & childBound, const kDTree & parent) : kDTree(childBound, parent), splitDimension(0) {
}

ABTree::~ABTree() {
}

void ABTree::calculateSplittingPlane(uint32_t & numFirstChild, uint32_t & numSecondChild) {
	float bestScore = std::numeric_limits<float>::max();
	for (unsigned char dim = 0; dim < 3; ++dim) {
		const uint32_t size = static_cast<uint32_t>(sorted[dim].size());
		const float fSize = static_cast<float> (size);
		/*
		 * The following strategy uses the "Statistically Weighted
		 * Multisampling" from page 431 and the functions for splitting plane
		 * selection from page 427.
		 */
		float sum = 0.0f;
		float squareSum = 0.0f;
		for (uint_fast32_t i = 0; i < size; ++i) {
			float value = triangleStorage->at(sorted[dim][i]).getMax(dim);
			sum += value;
			squareSum += value * value;
		}
		float mean = sum / fSize;
		// Take the absolute value because of rounding errors.
		float deviation = std::sqrt(std::abs(squareSum / fSize - mean * mean));


		// Sample twenty planes inside the standard deviation around the mean.
		for (short num = -10; num <= 10; ++num) {
			float possibleValue = mean + (static_cast<float> (num) / 10.0f)
					* deviation;
			// Evaluate the score for the splitting plane.
			uint32_t numCuts = 0;
			uint32_t numFrontFaces = 0;
			uint32_t numBackFaces = 0;
			for (uint_fast32_t index = 0; index < size; ++index) {
				float minPos, maxPos;
				if (needCut(triangleStorage->at(sorted[dim][index]), dim,
						possibleValue, minPos, maxPos)) {
					++numCuts;
				}
				if (maxPos < possibleValue) {
					++numFrontFaces;
				} else {
					++numBackFaces;
				}
			}
			const Geometry::Box & box(getBound());
			// Value near zero indicates that longest axis is split.
			float f1 = 1.0f - box.getExtent(static_cast<Geometry::dimension_t>(dim)) / box.getExtentMax();
			float w1 = 1.0f;
			// Value near zero indicates that volume is split equally.
			float splitPercent = (possibleValue - box.getMin(static_cast<Geometry::dimension_t>(dim))) / box.getExtent(static_cast<Geometry::dimension_t>(dim));
			float f2 = 2.0f * std::abs(0.5f - splitPercent);
			float w2 = 1.0f;
			// Value near zero indicates that triangles are split equally.
			float f3 = std::abs(static_cast<float> (numFrontFaces)
					- static_cast<float> (numBackFaces)) / fSize;
			float w3 = 3.0f;
			// Value near zero indicates that only few triangles are cut.
			float f4 = static_cast<float> (numCuts) / fSize;
			float w4 = 10.0f;
			// Smaller nodes should be split more equally.
			if (size < 3 * maxTrianglesPerNode) {
				w1 = 2.0f;
				w2 = 2.0f;
				w3 = 5.0f;
				w4 = 1.0f;
			}

			float score = w1 * f1 + w2 * f2 + w3 * f3 + w4 * f4;
			if (score < bestScore) {
				bestScore = score;
				splitDimension = dim;
				splitValue = possibleValue;
				numFirstChild = numFrontFaces;
				numSecondChild = numBackFaces;
			}
			/*std::cout << "dim=" << (int) dim << " size=" << size << " value="
			 << possibleValue << " score=" << score << " f1=" << f1
			 << " f2=" << f2 << " f3=" << f3 << " f4=" << f4
			 << std::endl;*/
		}
	}
}

}
}

#endif /* MINSG_EXT_TRIANGLETREES */
