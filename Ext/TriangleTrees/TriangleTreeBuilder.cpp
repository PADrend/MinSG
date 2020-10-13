/*
	This file is part of the MinSG library extension TriangleTrees.
	Copyright (C) 2009-2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_TRIANGLETREES

#include "TriangleTreeBuilder.h"
#include "TriangleAccessor.h"
#include "TriangleTree.h"
#include "../../Core/Nodes/GeometryNode.h"
#include "../../Core/Nodes/ListNode.h"
#include <Geometry/Matrix4x4.h>
#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Mesh/MeshDataStrategy.h>
#include <Rendering/Mesh/MeshVertexData.h>
#include <Rendering/Mesh/VertexAttribute.h>
#include <Rendering/Mesh/VertexAttributeIds.h>
#include <Rendering/Mesh/VertexDescription.h>
#include <Rendering/MeshUtils/MeshUtils.h>
#include <Util/References.h>
#include <Util/Timer.h>
#include <Util/Utils.h>
#include <algorithm>
#include <cstddef>
#include <deque>
#include <iostream>
#include <stdexcept>
#include <cstdint>
#include <vector>
#ifdef _OPENMP
#include <omp.h>
#endif

namespace MinSG {
namespace TriangleTrees {

Rendering::Mesh * Builder::mergeGeometry(const std::vector<GeometryNode *> & geoNodes) {
	std::deque<Util::Reference<Rendering::Mesh>> meshes;
	std::deque<Geometry::Matrix4x4> transformations;
	std::deque<Rendering::VertexDescription> vertexDescriptions;

	// Collect the meshes.
	for(const auto & geoNode : geoNodes) {
		Rendering::Mesh * mesh = geoNode->getMesh();
		if(mesh != nullptr) {
			if (mesh->getDrawMode() != Rendering::Mesh::DRAW_TRIANGLES) {
				throw std::invalid_argument("Cannot handle meshes without a triangle list.");
			}
			const Rendering::VertexDescription & vertexDesc = mesh->getVertexDescription();
			const Rendering::VertexAttribute & posAttr = vertexDesc.getAttribute(Rendering::VertexAttributeIds::POSITION);
			if (posAttr.getNumValues() != 3) {
				throw std::invalid_argument("Cannot handle vertices where the number of coordinates is not three.");
			}

			const Geometry::Matrix4x4 & transform = geoNode->getWorldTransformationMatrix();
			// Store a copy here.
			meshes.push_back(mesh->clone());
			transformations.push_back(transform);
			vertexDescriptions.push_back(vertexDesc);
		}
	}

	// Find a new vertex format for preserving all data.
	Rendering::VertexDescription vertexDesc = Rendering::MeshUtils::uniteVertexDescriptions(vertexDescriptions);

	// Convert all meshes to the new vertex format.
	std::deque<Rendering::Mesh *> meshPointers;
	for(const auto & mesh : meshes) {
		meshPointers.push_back(mesh.get());
		Rendering::MeshVertexData & oldData = mesh->openVertexData();
		std::unique_ptr<Rendering::MeshVertexData> newData(Rendering::MeshUtils::convertVertices(oldData, vertexDesc));
		oldData.swap(*newData.get());
	}

	// Merge the meshes into a single mesh.
	Util::Reference<Rendering::Mesh> mesh = Rendering::MeshUtils::combineMeshes(meshPointers, transformations);

	// Free the memory.
	meshPointers.clear();
	meshes.clear();

	if(mesh.isNull()) {
		throw std::runtime_error("Combining the meshes failed.");
	}

	// Optimize the mesh.
	mesh = Rendering::MeshUtils::eliminateZeroAreaTriangles(mesh.get());

	Rendering::MeshUtils::eliminateDuplicateVertices(mesh.get());

	mesh = Rendering::MeshUtils::eliminateUnusedVertices(mesh.get());

	return mesh.detachAndDecrease();
}

Node * Builder::buildMinSGTree(Rendering::Mesh * mesh, Builder & builder) {
	std::unique_ptr<TriangleTree> tree(builder.buildTriangleTree(mesh));

#ifdef MINSG_PROFILING
	Util::Timer timer;
	timer.reset();
#endif

	Node * node = convert(tree.get(), mesh->getVertexDescription());

#ifdef MINSG_PROFILING
	timer.stop();
	std::cout << "Profiling: Conversion to MinSG tree: " << timer.getMilliseconds() << " ms" << std::endl;
	Util::Utils::outputProcessMemory();
	const uint32_t meshTriangles = mesh->getPrimitiveCount();
	const uint32_t treeTriangles = tree->countTriangles();
	const uint32_t innerTriangles = tree->countInnerTriangles();
	const uint32_t outsideTriangles = tree->countTrianglesOutside();
	std::cout << "Profiling: Original mesh has " << meshTriangles << " triangles" << std::endl;
	std::cout << "Profiling: Tree has " << treeTriangles << " triangles";
	std::cout << " (" << static_cast<float>(treeTriangles) / static_cast<float>(meshTriangles) * 100.0f << "%)" << std::endl;
	std::cout << "Profiling: " << innerTriangles << " triangles in inner nodes";
	std::cout << " (" << static_cast<float>(innerTriangles) / static_cast<float>(treeTriangles) * 100.0f << "%)" << std::endl;
	std::cout << "Profiling: " << outsideTriangles << " triangles outside of bounding boxes";
	std::cout << " (" << static_cast<float>(outsideTriangles) / static_cast<float>(treeTriangles) * 100.0f << "%)" << std::endl;
#endif

	return node;
}

Node * Builder::convert(const TriangleTree * treeNode, const Rendering::VertexDescription & vertexDesc) {
	if (treeNode == nullptr) {
		return nullptr;
	}
	GeometryNode * geoNode = nullptr;
	const size_t num = treeNode->getTriangleCount();
	if (num > 0) {
		auto mesh = new Rendering::Mesh(vertexDesc, 3 * num, 3 * num);
		uint8_t * vPos = mesh->openVertexData().data();
		uint32_t * iPos = mesh->openIndexData().data();
		const size_t stride = vertexDesc.getVertexSize();
		for (size_t i = 0; i < num; ++i) {
			const TriangleAccessor & triangle = treeNode->getTriangle(i);
			for (unsigned char v = 0; v < 3; ++v) {
				const uint8_t * sourceVertex = triangle.getVertexData(v);
				std::copy(sourceVertex, sourceVertex + stride, vPos);

				*iPos = 3 * i + v;
				++iPos;

				vPos += stride;
			}
		}
		Rendering::MeshUtils::eliminateDuplicateVertices(mesh);
		Rendering::MeshUtils::optimizeIndices(mesh);

		geoNode = new GeometryNode;
		geoNode->setMesh(mesh);
	}
COMPILER_WARN_PUSH
COMPILER_WARN_OFF_CLANG(-Wunknown-pragmas)
	if (!treeNode->isLeaf()) {
		auto list = new ListNode;
#pragma omp critical (attributes)
		{
			treeNode->fetchAttributes(list);
		}
		const auto treeChildren = treeNode->getChildren();
		const uint32_t childCount = treeChildren.size();
		std::vector<Node *> children;
		children.reserve(childCount);
#pragma omp parallel for
		for(int_fast32_t c = 0; c < childCount; ++c) {
			Node * newChild = convert(treeChildren[c], vertexDesc);
#pragma omp critical
			{
				if(newChild != nullptr) {
					children.push_back(newChild);
				}
			}
		}
		for(const auto & newChild : children) {
			list->addChild(newChild);
		}
		if (geoNode != nullptr) {
			list->addChild(geoNode);
		}
		return list;
	}
	else {
		return geoNode;
	}
COMPILER_WARN_POP
}

}
}

#endif /* MINSG_EXT_TRIANGLETREES */
