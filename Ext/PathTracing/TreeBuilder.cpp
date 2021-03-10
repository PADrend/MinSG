/*
	This file is part of the MinSG library extension PathTracing.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2017 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_PATHTRACING

#include "TreeBuilder.h"
#include "ExtTriangle.h"
#include "Material.h"
#include "../../Helper/Helper.h"
#include "../../Helper/StdNodeVisitors.h"
#include "../../Core/Nodes/GeometryNode.h"
#include "../../Core/Nodes/GroupNode.h"
#include "../TriangleTrees/SolidTree.h"
#include "../TriangleTrees/TriangleAccessor.h"
#include "../TriangleTrees/TriangleTree.h"
#include "../TriangleTrees/ABTreeBuilder.h"
#include <Geometry/Box.h>
#include <Geometry/Triangle.h>
#include <Geometry/Vec3.h>
#include <Geometry/Vec2.h>
#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Mesh/VertexDescription.h>
#include <Rendering/Mesh/MeshVertexData.h>
#include <Rendering/Mesh/MeshIndexData.h>
#include <Rendering/Mesh/VertexAttribute.h>
#include <Rendering/Mesh/VertexAttributeIds.h>
#include <Rendering/Mesh/VertexAttributeAccessors.h>
#include <Rendering/MeshUtils/TriangleAccessor.h>
#include <Rendering/MeshUtils/MeshUtils.h>
#include <Util/Graphics/PixelAccessor.h>
#include <stdexcept>

namespace MinSG {
namespace PathTracing {
using namespace TriangleTrees;
using namespace Rendering;

inline Geometry::Vec3 colorToVec(const Util::Color4f& col) {
	return {col.r(), col.g(), col.b()};
}

SolidTree_ExtTriangle buildSolidExtTree(GroupNode * scene, std::vector<std::unique_ptr<Material>>& materialLibrary) {
	const auto collectedNodes = collectNodes<GeometryNode>(scene);
	const std::vector<GeometryNode *> geoNodes(collectedNodes.cbegin(), collectedNodes.cend());

	std::deque<Util::Reference<Rendering::Mesh>> meshes;
	std::deque<Rendering::Mesh *> meshPointers;
	std::deque<Geometry::Matrix4x4> transformations;

	// The new vertex format needs POSITION, NORMAL, COLOR, TEXCOORD0 and GEO_NODE_ID
	static const Util::StringIdentifier GEO_NODE_ID("geoNodeId");
	Rendering::VertexDescription targetVertexDesc;
	targetVertexDesc.appendPosition3D();
	targetVertexDesc.appendNormalByte();
	targetVertexDesc.appendColorRGBAByte();
	targetVertexDesc.appendTexCoord(0);
	const auto & geoNodeIdAttr = targetVertexDesc.appendUnsignedIntAttribute(GEO_NODE_ID, 1);

	// Collect the meshes.
	for(uint32_t geoNodeId = 0; geoNodeId < geoNodes.size(); ++geoNodeId) {
		Rendering::Mesh * inputMesh = geoNodes[geoNodeId]->getMesh();
		if(inputMesh != nullptr) {
			if(inputMesh->getDrawMode() != Rendering::Mesh::DRAW_TRIANGLES) {
				throw std::invalid_argument("Cannot handle meshes without a triangle list.");
			}
			const Rendering::VertexDescription & vertexDesc = inputMesh->getVertexDescription();
			const Rendering::VertexAttribute & posAttr = vertexDesc.getAttribute(Rendering::VertexAttributeIds::POSITION);
			if(posAttr.getComponentCount() != 3) {
				throw std::invalid_argument("Cannot handle vertices where the number of coordinates is not three.");
			}

			const Geometry::Matrix4x4 & transform = geoNodes[geoNodeId]->getWorldTransformationMatrix();
			// Store a copy here.
			meshes.push_back(inputMesh->clone());
			auto currentMesh = meshes.back()->clone();
			Rendering::MeshUtils::shrinkMesh(currentMesh);
			meshPointers.push_back(currentMesh);
			transformations.push_back(transform);

			// Convert mesh to the new vertex format.
			{
				Rendering::MeshVertexData & oldData = currentMesh->openVertexData();
				std::unique_ptr<Rendering::MeshVertexData> newData(Rendering::MeshUtils::convertVertices(oldData, targetVertexDesc));
				oldData.swap(*newData.get());
			}

			// Set GEO_NODE_ID.
			{
				auto & vertexData = currentMesh->openVertexData();
				auto acc = Rendering::UIntAttributeAccessor::create(vertexData, GEO_NODE_ID);
				for(uint32_t v = 0; v < vertexData.getVertexCount(); ++v) {
					acc->setValue(v, geoNodeId);
				}
			}
		}
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
	
	materialLibrary.clear();
	for(uint32_t i=0; i<geoNodes.size(); ++i) {
		materialLibrary.emplace_back(Material::createFromNode(geoNodes[i]));
	}
	// Create new triangle tree.
	TriangleTrees::ABTreeBuilder treeBuilder(32, 0.5f);
	std::unique_ptr<TriangleTrees::TriangleTree> triangleTree(treeBuilder.buildTriangleTree(mesh.get()));
	return convertTree(triangleTree.get(), mesh.get(), geoNodeIdAttr, geoNodes, materialLibrary);
}

SolidTree_ExtTriangle convertTree(const TriangleTree * treeNode,
										Rendering::Mesh* mesh,
									  const Rendering::VertexAttribute & idAttr,
									  const std::vector<GeometryNode *> & idLookup, 
										std::vector<std::unique_ptr<Material>>& materialLibrary) {
	if(treeNode == nullptr) {
		return SolidTree_ExtTriangle();
	}
	
	auto & vertexData = mesh->openVertexData();
	const auto tAcc = MeshUtils::TriangleAccessor::create(mesh);
	const auto pAcc = PositionAttributeAccessor::create(vertexData, VertexAttributeIds::POSITION);
	const auto nAcc = NormalAttributeAccessor::create(vertexData, VertexAttributeIds::NORMAL);
	const auto cAcc = ColorAttributeAccessor::create(vertexData, VertexAttributeIds::COLOR);
	const auto tcAcc = TexCoordAttributeAccessor::create(vertexData, VertexAttributeIds::TEXCOORD0);
	const auto idAcc = UIntAttributeAccessor::create(vertexData, idAttr.getName());
	
	SolidTree_ExtTriangle::triangles_t triangles;
	const auto triangleCount = treeNode->getTriangleCount();
	triangles.reserve(triangleCount);
	for(uint32_t t = 0; t < triangleCount; ++t) {
		const auto & triangleAccessor = treeNode->getTriangle(t);
		
		uint32_t idxA, idxB, idxC;
		std::tie(idxA, idxB, idxC) = tAcc->getIndices(triangleAccessor.getTriangleIndex());
		
		const auto idA = idAcc->getValue(idxA);
		const auto idB = idAcc->getValue(idxB);
		const auto idC = idAcc->getValue(idxC);
		if(idA != idB || idA != idC) {
			throw std::logic_error("A triangle cannot belong to different GeometryNodes.");
		}
		if(idA >= idLookup.size()) {
			throw std::logic_error("GeometryNode identifiers cannot be resolved.");
		}
		
		ExtTriangle tri;
		tri.pos = tAcc->getTriangle(triangleAccessor.getTriangleIndex());
		tri.normal.setVertexA(nAcc->getNormal(idxA));
		tri.normal.setVertexB(nAcc->getNormal(idxB));
		tri.normal.setVertexC(nAcc->getNormal(idxC));
		tri.color.setVertexA(colorToVec(cAcc->getColor4f(idxA)));
		tri.color.setVertexB(colorToVec(cAcc->getColor4f(idxB)));
		tri.color.setVertexC(colorToVec(cAcc->getColor4f(idxC)));
		tri.texCoord.setVertexA(tcAcc->getCoordinate(idxA));
		tri.texCoord.setVertexB(tcAcc->getCoordinate(idxB));
		tri.texCoord.setVertexC(tcAcc->getCoordinate(idxC));		
		tri.source = idLookup[idA];
		Material* mat = materialLibrary[idA].get();		
		tri.material = mat;
		triangles.emplace_back(tri);
	}

	SolidTree_ExtTriangle::children_t children;
	if(!treeNode->isLeaf()) {
		const auto treeChildren = treeNode->getChildren();
		children.reserve(treeChildren.size());
		for(const auto & child : treeChildren) {
			children.emplace_back(convertTree(child, mesh, idAttr, idLookup, materialLibrary));
		}
	}
	return SolidTree_ExtTriangle(treeNode->getBound(),
						std::move(children),
						std::move(triangles));
}

}
}

#endif /* MINSG_EXT_PATHTRACING */
