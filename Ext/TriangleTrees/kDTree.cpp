/*
	This file is part of the MinSG library extension TriangleTrees.
	Copyright (C) 2009-2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_TRIANGLETREES

#include "kDTree.h"
#include "TriangleAccessor.h"
#include <Geometry/Box.h>
#include <Geometry/Definitions.h>
#include <Rendering/Mesh/Mesh.h>
#include <Util/AttributeProvider.h>
#include <Util/GenericAttribute.h>
#include <Util/Numeric.h>
#include <Util/Macros.h>
#include <algorithm>
#include <cassert>
#include <functional>
#include <set>
#include <stdexcept>
#ifdef _OPENMP
#include <omp.h>
#endif

namespace MinSG {
namespace TriangleTrees {

/**
 * Compare the maximum coordinates of the given triangles in
 * the dimension given by the constructor parameter. This is used
 * for STL sort functions.
 */
class Comparator {
	public:
		Comparator(const std::vector<TriangleAccessor> & p_triangleStorage, const uint_fast8_t p_dimension) :
			m_triangleStorage(p_triangleStorage), m_dimension(p_dimension) {
		}
		bool operator()(uint32_t first, uint32_t second) const {
			return m_triangleStorage[first].getMax(m_dimension) < m_triangleStorage[second].getMax(m_dimension);
		}
	private:
		const std::vector<TriangleAccessor> & m_triangleStorage;
		const uint_fast8_t m_dimension;
};

kDTree::kDTree(Rendering::Mesh * mesh, uint32_t trianglesPerNode, float allowedBBEnlargement) :
		TriangleTree(mesh), splitValue(0.0f), firstChild(), secondChild(),
		maxTrianglesPerNode(trianglesPerNode), maxBoundingBoxEnlargement(allowedBBEnlargement) {
	// Reserve space in vector to prevent reallocation.
	const uint32_t size = mesh->getPrimitiveCount();
	triangleStorage = new std::vector<TriangleAccessor>;
	triangleStorage->reserve(size);
	for(uint_fast32_t i = 0; i < size; ++i) {
		triangleStorage->emplace_back(mesh, i);
	}

COMPILER_WARN_PUSH
COMPILER_WARN_OFF_CLANG(-Wunknown-pragmas)
#pragma omp parallel for schedule(static,1) num_threads(3)
	for (int_fast8_t dim = 0; dim < 3; ++dim) {
		// Create one array and sort it.
		sorted[dim].reserve(size);
		for (uint_fast32_t i = 0; i < size; ++i) {
			sorted[dim].push_back(i);
		}
		Comparator comparator(*triangleStorage, dim);
		sort(sorted[dim].begin(), sorted[dim].end(), comparator);
	}
COMPILER_WARN_POP
}

kDTree::kDTree(const Geometry::Box & childBound, const kDTree & parent) :
		TriangleTree(childBound, parent), triangleStorage(parent.triangleStorage), splitValue(0.0f), firstChild(), secondChild(),
		maxTrianglesPerNode(parent.maxTrianglesPerNode), maxBoundingBoxEnlargement(parent.maxBoundingBoxEnlargement) {
}

kDTree::~kDTree() {
	if (getLevel() == 0) {
		// Only root node has to free the storage.
		delete triangleStorage;
	}
}

void kDTree::calculateSplittingPlane(uint32_t & numFirstChild, uint32_t & numSecondChild) {
	// Split the triangles in two equally sized parts.
	unsigned char splitDimension = getSplitDimension();
	const uint32_t size = sorted[splitDimension].size();
	uint32_t half = size / 2; // Integer division = floor
	splitValue = triangleStorage->at(sorted[splitDimension][half]).getMax(splitDimension);
	// The loop handles the case when there are equal values.
	// The split value remains the same.
	while (half >= 1
			&& !(triangleStorage->at(sorted[splitDimension][half - 1]).getMax(splitDimension) < splitValue)) {
		--half;
	}

	numFirstChild = half;
	numSecondChild = size - half;
}

void kDTree::split() {
	// Only leaves can be split.
	if (!isLeaf()) {
		return;
	}

	assert(getLevel() < 200);

	uint32_t firstChildSize;
	uint32_t secondChildSize;
	calculateSplittingPlane(firstChildSize, secondChildSize);
	unsigned char splitDimension = getSplitDimension();
	assert(firstChildSize + secondChildSize == sorted[splitDimension].size());

	if (firstChildSize == 0 || secondChildSize == 0) {
		// No senseful split possible: Do not split.
		return;
	}

	const Geometry::Box & box(getBound());
	Geometry::Box firstChildBound(box);
	Geometry::Box secondChildBound(box);
	firstChildBound.setMax(static_cast<Geometry::dimension_t>(splitDimension), splitValue);
	secondChildBound.setMin(static_cast<Geometry::dimension_t>(splitDimension), splitValue);


	// Create children.
	firstChild.reset(createChild(firstChildBound, *this));
	secondChild.reset(createChild(secondChildBound, *this));

	const float secondChildInitialExtent = secondChild->getBound().getExtent(static_cast<Geometry::dimension_t>(splitDimension));

	// Map holds the deleted triangles.
	// They are used when distributing the triangles to the children.
	std::set<uint32_t> deletedTriangles;
	// Find triangles which are cut by the splitting plane.
	for (uint_fast32_t i = 0; i < sorted[splitDimension].size(); ++i) {
		const uint32_t index = sorted[splitDimension][i];
		const TriangleAccessor & triangle = triangleStorage->at(index);
		float minPos;
		float maxPos;
		// Check if the triangle has to be cut.
		if (needCut(triangle, splitDimension, splitValue, minPos, maxPos)) {
			// First check if the bounding box can be enlarged.
			if (!(maxPos < splitValue) && !(minPos < splitValue - maxBoundingBoxEnlargement * secondChildInitialExtent)) {
				// Enlarge only the second bounding box.
				Geometry::Box secondChildBox = secondChild->getBound();
				if (minPos < secondChildBox.getMin(static_cast<Geometry::dimension_t>(splitDimension))) {
					secondChildBox.setMin(static_cast<Geometry::dimension_t>(splitDimension), minPos);
					secondChild->setBound(secondChildBox);
				}
			} else {
				// Prevent triangle from being distributed to children.
				deletedTriangles.insert(index);
				// Counter for second child is decreased below.
			}
		}
	}

	// Cut triangles would have been assigned to second child.
	// => Subtract here.
	secondChildSize -= deletedTriangles.size();
	assert(firstChildSize + secondChildSize == sorted[splitDimension].size() - deletedTriangles.size());

	// Distribute the triangles to the children.
	for (uint_fast8_t dim = 0; dim < 3; ++dim) {
		firstChild->sorted[dim].clear();
		firstChild->sorted[dim].reserve(firstChildSize);
		secondChild->sorted[dim].clear();
		secondChild->sorted[dim].reserve(secondChildSize);

#ifndef NDEBUG
		uint32_t firstCount = 0;
		uint32_t secondCount = 0;
#endif
		for (uint_fast32_t i = 0; i < sorted[dim].size(); ++i) {
			const uint32_t index = sorted[dim][i];
			const TriangleAccessor & triangle = triangleStorage->at(index);
			if (deletedTriangles.count(index) != 0) {
				// Do not insert deleted triangles.
				continue;
			}
			// Always using smaller!
			if (triangle.getMax(splitDimension) < splitValue) {
				firstChild->sorted[dim].push_back(index);
#ifndef NDEBUG
				++firstCount;
#endif
			} else {
				secondChild->sorted[dim].push_back(index);
#ifndef NDEBUG
				++secondCount;
#endif
			}
		}
#ifndef NDEBUG
		// Make sure everything is sorted correctly.
		for (uint_fast32_t i = 1; i < firstChild->sorted[dim].size(); ++i) {
			assert(!(triangleStorage->at(firstChild->sorted[dim][i - 1]).getMax(dim) > triangleStorage->at(firstChild->sorted[dim][i]).getMax(dim)));
		}
		for (uint_fast32_t i = 1; i < secondChild->sorted[dim].size(); ++i) {
			assert(!(triangleStorage->at(secondChild->sorted[dim][i - 1]).getMax(dim) > triangleStorage->at(secondChild->sorted[dim][i]).getMax(dim)));
		}
		assert(firstChild->sorted[dim].size() == firstCount);
		assert(secondChild->sorted[dim].size() == secondCount);
#endif
		// Sorted list not needed anymore.
		sorted[dim].clear();
		// Make sure that the memory is freed.
		sorted[dim].shrink_to_fit();
	}

	// Re-add the deleted triangles to this node.
	if (!deletedTriangles.empty()) {
		// Copy triangle indices.
		sorted[splitDimension].assign(deletedTriangles.begin(), deletedTriangles.end());
	}

	// Check if the children should be split.
COMPILER_WARN_PUSH
COMPILER_WARN_OFF_CLANG(-Wunknown-pragmas)
#pragma omp parallel sections
	{
#pragma omp section
		if (firstChild->shouldSplit()) {
			firstChild->split();
		}
#pragma omp section
		if (secondChild->shouldSplit()) {
			secondChild->split();
		}
	}
COMPILER_WARN_POP
}

bool kDTree::needCut(const TriangleAccessor & triangle, unsigned char splitDimension, float splitValue, float & minPos, float & maxPos) {
	// Get the extremal positions.
	minPos = triangle.getMin(splitDimension);
	maxPos = triangle.getMax(splitDimension);
	// If both extremal positions lie on the same side of the splitting plane
	// no cut is needed.
	return ((minPos < splitValue) ^ (maxPos < splitValue));
}

bool kDTree::contains(const TriangleAccessor & triangle) const {
	const Geometry::Box & box(getBound());
	for (unsigned char dim = 0; dim < 3; ++dim) {
		const float & min = box.getMin(static_cast<Geometry::dimension_t>(dim));
		const float & max = box.getMax(static_cast<Geometry::dimension_t>(dim));
		for (unsigned char v = 0; v < 3; ++v) {
			const float & value = triangle.getVertexPosition(v)[dim];
			if ((!Util::Numeric::equal(value, min) && value < min)
					|| (!Util::Numeric::equal(value, max) && value > max)) {
				return false;
			}
		}
	}
	return true;
}

const TriangleAccessor & kDTree::getTriangle(uint32_t index) const {
	if (index >= getTriangleCount()) {
		throw std::out_of_range("Parameter index out of range.");
	}
	return triangleStorage->at(sorted[getSplitDimension()][index]);
}

void kDTree::fetchAttributes(Util::AttributeProvider * container) const {
	container->setAttribute(Util::StringIdentifier("splitDimension"), Util::GenericAttribute::createNumber<uint16_t>(getSplitDimension()));
	container->setAttribute(Util::StringIdentifier("splitValue"), Util::GenericAttribute::createNumber(getSplitValue()));
}

}
}

#endif /* MINSG_EXT_TRIANGLETREES */
