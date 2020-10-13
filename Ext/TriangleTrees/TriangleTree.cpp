/*
	This file is part of the MinSG library extension TriangleTrees.
	Copyright (C) 2009-2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_TRIANGLETREES

#include "TriangleTree.h"
#include <Rendering/Mesh/Mesh.h>
#include <Rendering/MeshUtils/LocalMeshDataHolder.h>
#include <cstdint>
#ifdef _OPENMP
#include <omp.h>
#endif
#include <Util/Macros.h>

namespace Geometry {
template<typename value_t> class _Box;
typedef _Box<float> Box;
}
namespace MinSG {
namespace TriangleTrees {
class TriangleAccessor;

TriangleTree::TriangleTree(Rendering::Mesh * mesh) : 
		meshHolder(new Rendering::MeshUtils::LocalMeshDataHolder(mesh)), 
		bound(mesh->getBoundingBox()), 
		level(0) {
}

TriangleTree::TriangleTree(Geometry::Box childBound, const TriangleTree & parent) :
		meshHolder(), 
		bound(std::move(childBound)), 
		level(parent.getLevel() + 1) {
}

TriangleTree::~TriangleTree() = default;

uint32_t TriangleTree::countTriangles() const {
	uint32_t sum = getTriangleCount();
	if (!isLeaf()) {
		const auto children = getChildren();
		const uint32_t childCount = children.size();
COMPILER_WARN_PUSH
COMPILER_WARN_OFF_CLANG(-Wunknown-pragmas)
#pragma omp parallel for reduction(+:sum)
		for(int_fast32_t c = 0; c < childCount; ++c) {
			sum += children[c]->countTriangles();
		}
	}
COMPILER_WARN_POP 
	return sum;
}

uint32_t TriangleTree::countInnerTriangles() const {
	if(isLeaf()) {
		return 0;
	}
	uint32_t sum = getTriangleCount();
	const auto children = getChildren();
	const uint32_t childCount = children.size();
COMPILER_WARN_PUSH
COMPILER_WARN_OFF_CLANG(-Wunknown-pragmas)
#pragma omp parallel for reduction(+:sum)
	for(int_fast32_t c = 0; c < childCount; ++c) {
		sum += children[c]->countInnerTriangles();
	}
COMPILER_WARN_POP
	return sum;
}

uint32_t TriangleTree::countTrianglesOutside() const {
	uint32_t sum = 0;
	if (!isLeaf()) {
		const auto children = getChildren();
		const uint32_t childCount = children.size();
COMPILER_WARN_PUSH
COMPILER_WARN_OFF_CLANG(-Wunknown-pragmas)
#pragma omp parallel for reduction(+:sum)
		for(int_fast32_t c = 0; c < childCount; ++c) {
			sum += children[c]->countTrianglesOutside();
		}
COMPILER_WARN_POP
	}
	const uint32_t triangleCount = getTriangleCount();
	for (uint_fast32_t i = 0; i < triangleCount; ++i) {
		const TriangleAccessor & triangle = getTriangle(i);
		if (!contains(triangle)) {
			++sum;
		}
	}
	return sum;
}

}
}

#endif /* MINSG_EXT_TRIANGLETREES */
