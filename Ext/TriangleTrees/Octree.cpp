/*
	This file is part of the MinSG library extension TriangleTrees.
	Copyright (C) 2009-2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_TRIANGLETREES

#include "Octree.h"
#include "TriangleAccessor.h"
#include <Geometry/Box.h>
#include <Geometry/Definitions.h>
#include <Geometry/Vec3.h>
#include <Rendering/Mesh/Mesh.h>
#include <Util/AttributeProvider.h>
#include <Util/GenericAttribute.h>
#include <Util/Macros.h>
#include <cassert>
#include <iosfwd>
#include <stdexcept>
#include <cstdint>
#ifdef _OPENMP
#include <omp.h>
#endif

namespace MinSG {
namespace TriangleTrees {

Octree::Octree(Rendering::Mesh * mesh, uint32_t trianglesPerNode, float enlargementFactor) : TriangleTree(mesh),
		children(), maxTrianglesPerNode(trianglesPerNode), looseFactor(enlargementFactor) {
	// Reserve space in vector to prevent reallocation.
	const uint32_t size = mesh->getPrimitiveCount();
	triangleStorage.reserve(size);
	for(uint_fast32_t i = 0; i < size; ++i) {
		triangleStorage.emplace_back(mesh, i);
	}

	const Geometry::Box & meshBB = mesh->getBoundingBox();
	const float maxExtent = meshBB.getExtentMax();
	// Create cubic box.
	Geometry::Box looseBound(meshBB.getMinX(), meshBB.getMinX() + maxExtent,
							   meshBB.getMinY(), meshBB.getMinY() + maxExtent,
							   meshBB.getMinZ(), meshBB.getMinZ() + maxExtent);
	looseBound.resizeRel(looseFactor);
	setBound(looseBound);
}

Octree::Octree(const Geometry::Box & childBound, const Octree & parent) : TriangleTree(childBound, parent),
		triangleStorage(), children(), maxTrianglesPerNode(parent.maxTrianglesPerNode), looseFactor(parent.looseFactor) {
}

Octree::~Octree() = default;

void Octree::split() {
	// Only leaves can be split.
	if (!isLeaf()) {
		return;
	}

	assert(getLevel() < 200);

	Geometry::Box looseBound(getBound());
	looseBound.resizeRel(1.0f / looseFactor);

	// Create children.
	children.resize(8);
	for(uint_fast8_t c = 0; c < 8; ++c) {
		Geometry::Box childBounds(looseBound.getCenter(), looseBound.getCorner(static_cast<Geometry::corner_t>(c)));
		childBounds.resizeRel(looseFactor);
		children[c].reset(createChild(childBounds, *this));
	}

	std::vector<TriangleAccessor> innerStorage;
	const uint32_t numTriangles = triangleStorage.size();
	for(uint_fast32_t t = 0; t < numTriangles; ++t) {
		const TriangleAccessor & triangle = triangleStorage[t];
		// Check into which child node the center of the triangle fits.
		const Geometry::Vec3 center = (Geometry::Vec3f(triangle.getVertexPosition(0))
										+ Geometry::Vec3f(triangle.getVertexPosition(1))
										+ Geometry::Vec3f(triangle.getVertexPosition(2))) / 3;
		const auto corner = static_cast<size_t>(looseBound.getOctant(center));
		if(children[corner]->contains(triangle)) {
			children[corner]->triangleStorage.push_back(triangle);
		} else {
			innerStorage.push_back(triangle);
		}
	}
	assert(
		children[0]->triangleStorage.size()
		+ children[1]->triangleStorage.size()
		+ children[2]->triangleStorage.size()
		+ children[3]->triangleStorage.size()
		+ children[4]->triangleStorage.size()
		+ children[5]->triangleStorage.size()
		+ children[6]->triangleStorage.size()
		+ children[7]->triangleStorage.size()
		+ innerStorage.size() == triangleStorage.size()
	);

	if(triangleStorage.size() == innerStorage.size()) {
		// All child nodes are empty.
		children.clear();
		children.shrink_to_fit();
		return;
	}

	triangleStorage.swap(innerStorage);

	// Check if the children should be split.
COMPILER_WARN_PUSH
COMPILER_WARN_OFF_CLANG(-Wunknown-pragmas)
#pragma omp parallel for schedule(static,2) num_threads(3)
	for(int_fast8_t c = 0; c < 8; ++c) {
		if (children[c]->shouldSplit()) {
			children[c]->split();
		}
	}
COMPILER_WARN_POP
}

bool Octree::contains(const TriangleAccessor & triangle) const {
	const Geometry::Box & looseBound(getBound());
	return looseBound.contains(Geometry::Vec3f(triangle.getVertexPosition(0)))
			&& looseBound.contains(Geometry::Vec3f(triangle.getVertexPosition(1)))
			&& looseBound.contains(Geometry::Vec3f(triangle.getVertexPosition(2)));
}

const TriangleAccessor & Octree::getTriangle(uint32_t index) const {
	if (index >= triangleStorage.size()) {
		throw std::out_of_range("Parameter index out of range.");
	}
	return triangleStorage[index];
}

void Octree::fetchAttributes(Util::AttributeProvider * container) const {
	container->setAttribute(Util::StringIdentifier("looseFactor"), Util::GenericAttribute::createNumber<float>(looseFactor));
	std::ostringstream stream;
	stream << getBound();
	container->setAttribute(Util::StringIdentifier("looseBox"), Util::GenericAttribute::createString(stream.str()));
}

}
}

#endif /* MINSG_EXT_TRIANGLETREES */
