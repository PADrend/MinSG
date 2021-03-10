/*
	This file is part of the MinSG library extension TriangleTrees.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_TRIANGLETREES

#include "Conversion.h"
#include "SolidTree.h"
#include "TriangleAccessor.h"
#include "TriangleTree.h"
#include <Geometry/Box.h>
#include <Geometry/Triangle.h>
#include <Geometry/Vec3.h>
#include <Rendering/Mesh/VertexAttribute.h>
#include <stdexcept>

namespace MinSG {
namespace TriangleTrees {

SolidTree_3f convertTree(const TriangleTree * treeNode) {
	if(treeNode == nullptr) {
		return SolidTree_3f();
	}

	SolidTree_3f::triangles_t triangles;
	const auto triangleCount = treeNode->getTriangleCount();
	triangles.reserve(triangleCount);
	for(uint32_t t = 0; t < triangleCount; ++t) {
		const auto & triangleAccessor = treeNode->getTriangle(t);
		triangles.emplace_back(triangleAccessor.getTriangle());
	}

	SolidTree_3f::children_t children;
	if(!treeNode->isLeaf()) {
		const auto treeChildren = treeNode->getChildren();
		children.reserve(treeChildren.size());
		for(const auto & child : treeChildren) {
			children.emplace_back(convertTree(child));
		}
	}
	return SolidTree_3f(treeNode->getBound(),
						std::move(children),
						std::move(triangles));
}

SolidTree_3f_GeometryNode convertTree(const TriangleTree * treeNode,
									  const Rendering::VertexAttribute & idAttr,
									  const std::vector<GeometryNode *> & idLookup) {
	if(treeNode == nullptr) {
		return SolidTree_3f_GeometryNode();
	}

	SolidTree_3f_GeometryNode::triangles_t triangles;
	const auto triangleCount = treeNode->getTriangleCount();
	triangles.reserve(triangleCount);
	for(uint32_t t = 0; t < triangleCount; ++t) {
		const auto & triangleAccessor = treeNode->getTriangle(t);
		const auto idA = *reinterpret_cast<const uint32_t *>(triangleAccessor.getVertexData(0) + idAttr.getOffset());
		const auto idB = *reinterpret_cast<const uint32_t *>(triangleAccessor.getVertexData(1) + idAttr.getOffset());
		const auto idC = *reinterpret_cast<const uint32_t *>(triangleAccessor.getVertexData(2) + idAttr.getOffset());
		if(idA != idB || idA != idC) {
			throw std::logic_error("A triangle cannot belong to different GeometryNodes.");
		}
		if(idA >= idLookup.size()) {
			throw std::logic_error("GeometryNode identifiers cannot be resolved.");
		}
		triangles.emplace_back(triangleAccessor.getTriangle(), idLookup[idA]);
	}

	SolidTree_3f_GeometryNode::children_t children;
	if(!treeNode->isLeaf()) {
		const auto treeChildren = treeNode->getChildren();
		children.reserve(treeChildren.size());
		for(const auto & child : treeChildren) {
			children.emplace_back(convertTree(child, idAttr, idLookup));
		}
	}
	return SolidTree_3f_GeometryNode(treeNode->getBound(),
						std::move(children),
						std::move(triangles));
}

}
}

#endif /* MINSG_EXT_TRIANGLETREES */
