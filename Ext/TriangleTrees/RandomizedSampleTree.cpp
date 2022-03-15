/*
	This file is part of the MinSG library extension TriangleTrees.
	Copyright (C) 2009-2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_TRIANGLETREES

#include "RandomizedSampleTree.h"
#include "TriangleAccessor.h"
#include <Geometry/Box.h>
#include <Geometry/Vec3.h>
#include <Geometry/Triangle.h>
#include <Util/Numeric.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <random>
#include <cstdint>
#include <vector>

namespace MinSG {
namespace TriangleTrees {

static double calcTriangleArea(const TriangleAccessor & triangle) {
	return triangle.getTriangle().calcArea();
}

// One element of the population that will be sampled randomly.
struct TriangleEntity {
	double area;
	bool selected;

	/*TriangleEntity() : area(0.0), selected(false) {
	}*/
	TriangleEntity(double _area) : area(_area), selected(false) {
	}
	bool operator<(const TriangleEntity & other) const {
		return area < other.area;
	}
};
    
void RandomizedSampleTree::createSample() {
    createSample(calcTriangleArea);
}

void RandomizedSampleTree::createSample(const std::function<double(const TriangleAccessor &)> & calcTriangleWeight) {
    if(!calcTriangleWeight)
        return;
        
	if(isLeaf()) {
		return;
	}

	// Store casted pointers here, because they are needed multiple times in this function.
	RandomizedSampleTree * rstChildren[8] = {
												static_cast<RandomizedSampleTree *>(children[0].get()),
												static_cast<RandomizedSampleTree *>(children[1].get()),
												static_cast<RandomizedSampleTree *>(children[2].get()),
												static_cast<RandomizedSampleTree *>(children[3].get()),
												static_cast<RandomizedSampleTree *>(children[4].get()),
												static_cast<RandomizedSampleTree *>(children[5].get()),
												static_cast<RandomizedSampleTree *>(children[6].get()),
												static_cast<RandomizedSampleTree *>(children[7].get())
	};

	uint32_t sumChildTriangles = 0;
	for(auto & elem : rstChildren) {
		elem->createSample(calcTriangleWeight);
		sumChildTriangles += static_cast<uint32_t>(elem->triangleStorage.size());
	}

	if(sumChildTriangles == 0) {
		// There is nothing to sample from direct child nodes.
		return;
	}

	// Sampling population with accumulated triangle areas.
	std::vector<TriangleEntity> population;
	population.reserve(sumChildTriangles);

	// See A(P(u)) in Section 4.1 of the original article
	double sumArea = 0.0;

	// Only sample from the triangles of the direct child nodes as suggested in section 5.2 of the original article.
	for(auto & elem : rstChildren) {
		const uint32_t triangleCount = static_cast<uint32_t>(elem->triangleStorage.size());
		for (uint_fast32_t t = 0; t < triangleCount; ++t) {
			sumArea += calcTriangleWeight(elem->triangleStorage[t]);
			population.emplace_back(sumArea);
		}
	}

	if(Util::Numeric::equal(sumArea, 0.0)) {
		return;
	}

	const uint32_t mu = static_cast<uint32_t>(std::ceil(calcSampleSize(sumArea)));

	static std::default_random_engine engine;
	std::uniform_real_distribution<double> distribution(0, sumArea);

	uint32_t numSelected = 0;
	for(uint_fast32_t s = 0; s < mu; ++s) {
		// Random sample from [0, sumArea].
		const double randomSample = distribution(engine);
		TriangleEntity & selectedTriangle = *std::upper_bound(population.begin(), population.end(), TriangleEntity(randomSample));
		if(!selectedTriangle.selected) {
			++numSelected;
			selectedTriangle.selected = true;
		}
	}

	// Move triangles from children into this node.
	triangleStorage.reserve(triangleStorage.size() + numSelected);

	uint_fast32_t p = 0;
	for(auto & elem : rstChildren) {
		const uint32_t triangleCount = static_cast<uint32_t>(elem->triangleStorage.size());
		std::vector<TriangleAccessor> newChildStorage;
		newChildStorage.reserve(triangleCount);
		for (uint_fast32_t t = 0; t < triangleCount; ++t) {
			if(population[p].selected) {
				triangleStorage.push_back(elem->triangleStorage[t]);
			} else {
				newChildStorage.push_back(elem->triangleStorage[t]);
			}
			++p;
		}
		elem->triangleStorage.swap(newChildStorage);
	}
}

double RandomizedSampleTree::calcSampleSize(double sumTriangleAreas) const {
	// Value from the original article.
	const double c = 13.8;

	// Bounding box is a cube. Therefore we can choose any side for the following calculation.
	const double sideArea = getBound().getExtentX() * getBound().getExtentX();

	// See Theorem 1.
	const double qu = std::ceil(sumTriangleAreas / sideArea);

	// Condition taken from original implementation.
	const double e = 2.7182818284590452354;
	if(qu < e) {
		return 1.0 + c * qu;
	} else {
		return qu * std::log(qu) + c * qu;
	}
}

}
}

#endif /* MINSG_EXT_TRIANGLETREES */
