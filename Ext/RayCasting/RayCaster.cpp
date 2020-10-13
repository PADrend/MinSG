/*
	This file is part of the MinSG library extension RayCasting.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_RAYCASTING

#include "RayCaster.h"
#include "../TriangleTrees/ABTreeBuilder.h"
#include "../TriangleTrees/Conversion.h"
#include "../TriangleTrees/SolidTree.h"
#include "../TriangleTrees/TriangleTree.h"
#include "../../Core/Nodes/GeometryNode.h"
#include "../../Core/Nodes/GroupNode.h"
#include "../../Core/NodeAttributeModifier.h"
#include "../../Helper/Helper.h"
#include "../../Helper/StdNodeVisitors.h"
#include <Geometry/BoxHelper.h>
#include <Geometry/BoxIntersection.h>
#include <Geometry/Line.h>
#include <Geometry/LineTriangleIntersection.h>
#include <Geometry/RayBoxIntersection.h>
#include <Geometry/Triangle.h>
#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Mesh/VertexAttributeIds.h>
#include <Rendering/Mesh/VertexDescription.h>
#include <Rendering/MeshUtils/MeshUtils.h>
#include <Util/ObjectExtension.h>
#include <Util/References.h>
#include <Util/StringIdentifier.h>
#include <Util/Utils.h>
#include <cstdint>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

#ifdef MINSG_EXT_RAYCASTING_PROFILING
#include "../Profiling/Logger.h"
#include "../Profiling/Profiler.h"
#include <fstream>

#define PROFILING_BEGIN(actionIdentifier, description) \
	auto actionIdentifier = context.profiler.beginTimeMemoryAction(description);
#define PROFILING_END(actionIdentifier) \
	context.profiler.endTimeMemoryAction(actionIdentifier);

#else /* MINSG_EXT_RAYCASTING_PROFILING */

#define PROFILING_BEGIN(actionIdentifier, description)
#define PROFILING_END(actionIdentifier)

#endif /* MINSG_EXT_RAYCASTING_PROFILING */

namespace MinSG {
namespace RayCasting {

static const auto idSolidGeoTree = NodeAttributeModifier::create("SolidGeoTree", NodeAttributeModifier::PRIVATE_ATTRIBUTE);

static bool hasSolidGeoTree(Node * node) {
	return Util::hasObjectExtension<TriangleTrees::SolidTree_3f_GeometryNode>(idSolidGeoTree, node);
}

static const TriangleTrees::SolidTree_3f_GeometryNode & getSolidGeoTree(Node * node) {
	return *Util::requireObjectExtension<TriangleTrees::SolidTree_3f_GeometryNode>(idSolidGeoTree, node);
}

static void storeSolidGeoTree(Node * node, TriangleTrees::SolidTree_3f_GeometryNode && solidTree) {
	Util::addObjectExtension<TriangleTrees::SolidTree_3f_GeometryNode>(idSolidGeoTree, node, std::move(solidTree));
}

static TriangleTrees::SolidTree_3f_GeometryNode buildSolidGeoTree(GroupNode * scene) {
	const auto collectedNodes = collectNodes<GeometryNode>(scene);
	const std::vector<GeometryNode *> geoNodes(collectedNodes.cbegin(), collectedNodes.cend());

	std::deque<Util::Reference<Rendering::Mesh>> meshes;
	std::deque<Rendering::Mesh *> meshPointers;
	std::deque<Geometry::Matrix4x4> transformations;

	// The new vertex format needs only POSITION and GEO_NODE_ID
	static const Util::StringIdentifier GEO_NODE_ID("geoNodeId");
	Rendering::VertexDescription targetVertexDesc;
	targetVertexDesc.appendPosition3D();
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
			if(posAttr.getNumValues() != 3) {
				throw std::invalid_argument("Cannot handle vertices where the number of coordinates is not three.");
			}

			const Geometry::Matrix4x4 & transform = geoNodes[geoNodeId]->getWorldTransformationMatrix();
			// Store a copy here.
			meshes.push_back(inputMesh->clone());
			auto currentMesh = meshes.back().get();
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
				Rendering::MeshVertexData & vertexData = currentMesh->openVertexData();
				const auto vertexSize = targetVertexDesc.getVertexSize();
				auto vertexDataPtr = vertexData.data() + geoNodeIdAttr.getOffset();
				for(uint32_t v = 0; v < vertexData.getVertexCount(); ++v) {
					*reinterpret_cast<uint32_t *>(vertexDataPtr + v * vertexSize) = geoNodeId;
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

	// Create new triangle tree.
	TriangleTrees::ABTreeBuilder treeBuilder(32, 0.5f);
	std::unique_ptr<TriangleTrees::TriangleTree> triangleTree(treeBuilder.buildTriangleTree(mesh.get()));
	return TriangleTrees::convertTree(triangleTree.get(), geoNodeIdAttr, geoNodes);
}

template<typename value_t>
class Context {
	public:
		typedef Geometry::_Vec3<value_t> vec3_t;
		typedef Geometry::_Ray<vec3_t> ray_t;
		typedef Geometry::Intersection::Slope<value_t> slope_t;

		const std::vector<slope_t> slopes;
		typename RayCaster<value_t>::intersection_packet_t results;

#ifdef MINSG_EXT_RAYCASTING_PROFILING
		std::unique_ptr<Profiling::LoggerTSV> tsvLogger;
		std::ofstream tsvLoggerStream;
		Profiling::Profiler profiler;
#endif /* MINSG_EXT_RAYCASTING_PROFILING */

		Context(const std::vector<ray_t> & rays) :
			slopes(rays.cbegin(), rays.cend()),
			results(rays.size(), 
					std::make_pair(nullptr, std::numeric_limits<value_t>::max())) {
#ifdef MINSG_EXT_RAYCASTING_PROFILING
			tsvLoggerStream.open(Util::Utils::createTimeStamp() + "_RayCasting.tsv");
			tsvLogger.reset(new Profiling::LoggerTSV(tsvLoggerStream));
			profiler.registerLogger(tsvLogger.get());
#endif /* MINSG_EXT_RAYCASTING_PROFILING */
		}

#ifdef MINSG_EXT_RAYCASTING_PROFILING
		~Context() {
			profiler.unregisterLogger(tsvLogger.get());
			tsvLogger.reset();
		}
#endif /* MINSG_EXT_RAYCASTING_PROFILING */
};

/**
 * Intersection query for a single ray associating the input (the ray) with the
 * output (the intersected object and the intersection distance).
 */
template<typename value_t>
class IntersectionQuery {
	public:
		typedef Geometry::_Vec3<value_t> vec3_t;
		typedef Geometry::_Ray<vec3_t> ray_t;
		typedef Geometry::Intersection::Slope<value_t> slope_t;
		typedef typename RayCaster<value_t>::intersection_t intersection_t;
	private:
		//! Read-only reference to the input entry stored in the context
		const slope_t & slope;
		//! Read-write output entry stored in the context
		intersection_t & result;

	public:
		//! Create new query from referenced objects
		IntersectionQuery(const slope_t & crefSlope, 
						  intersection_t & refResult) : 
			slope(crefSlope), result(refResult) {
		}

		value_t getDistance() const {
			return result.second;
		}
		GeometryNode * getObject() const {
			return result.first;
		}
		const ray_t & getRay() const {
			return slope.getRay();
		}
		const slope_t & getSlope() const {
			return slope;
		}

		void setDistance(value_t distance) const {
			result.second = distance;
		}
		void setObject(GeometryNode * object) const {
			result.first = object;
		}
};

#if defined(_MSC_VER)
#define UNUSED(name) __pragma(warning(suppress:4100)) name
#else
#define UNUSED(name) name __attribute__((unused))
#endif

template<typename value_t>
static void testTrianglesInNode(const TriangleTrees::SolidTree_3f_GeometryNode & node,
								const std::vector<IntersectionQuery<value_t>> & queries,
								Context<value_t> & UNUSED(context)) {
	PROFILING_BEGIN(trianglesAction, "Triangles test");

	for(const auto & triangleData : node.getTriangles()) {
		const auto & triangle = triangleData.first;
		GeometryNode * geoNode = triangleData.second;
		value_t t, u, v;
		for(const auto & query : queries) {
			const auto & ray = query.getRay();
			using namespace Geometry::Intersection;
			const bool intersection = getLineTriangleIntersection(ray, triangle, t, u, v);
			if(intersection && !(t < 0) && t < query.getDistance()) {
				query.setDistance(t);
				query.setObject(geoNode);
			}
		}
	}

	PROFILING_END(trianglesAction);
}

//! Tree traversal for ray casting
template<typename value_t>
static void visit(const TriangleTrees::SolidTree_3f_GeometryNode & node,
				  Context<value_t> & context,
				  const std::vector<IntersectionQuery<value_t>> & queries) {
	PROFILING_BEGIN(bbAction, "Bounding box test");

	const auto & worldBB = node.getBound();

	std::vector<IntersectionQuery<value_t>> nodeQueries;
	nodeQueries.reserve(queries.size());
	for(const auto & query : queries) {
		value_t t;
		const bool intersection = query.getSlope().getRayBoxIntersection(worldBB, t);
		/*
		 * Only check a node if it is nearer to the ray origin than the
		 * previous result. Do not check for negative t, because the
		 * ray origin can be located inside the box.
		 */
		if(intersection && t < query.getDistance()) {
			nodeQueries.emplace_back(query);
		}
	}

	PROFILING_END(bbAction);

	if(nodeQueries.empty()) {
		return;
	}
	testTrianglesInNode(node, nodeQueries, context);
	for(auto & child : node.getChildren()) {
		visit(child, context, nodeQueries);
	}
}

//! Tree traversal for ray casting with additional box intersection
template<typename value_t>
static void visit(const TriangleTrees::SolidTree_3f_GeometryNode & node,
				  Context<value_t> & context,
				  const std::vector<IntersectionQuery<value_t>> & queries,
				  const Geometry::Box & testBox) {
	PROFILING_BEGIN(bbAction, "Bounding box test");

	const auto & worldBB = node.getBound();
	if(!Geometry::Intersection::isBoxIntersectingBox(worldBB, testBox)) {
		return;
	}

	std::vector<IntersectionQuery<value_t>> nodeQueries;
	nodeQueries.reserve(queries.size());
	for(const auto & query : queries) {
		value_t t;
		const bool intersection = query.getSlope().getRayBoxIntersection(worldBB, t);
		/*
		 * Only check a node if it is nearer to the ray origin than the
		 * previous result. Do not check for negative t, because the
		 * ray origin can be located inside the box.
		 */
		if(intersection && t < query.getDistance()) {
			nodeQueries.emplace_back(query);
		}
	}

	PROFILING_END(bbAction);

	if(nodeQueries.empty()) {
		return;
	}
	testTrianglesInNode(node, nodeQueries, context);
	for(auto & child : node.getChildren()) {
		visit(child, context, nodeQueries, testBox);
	}
}

template<typename value_t>
typename RayCaster<value_t>::intersection_packet_t RayCaster<value_t>::castRays(
											GroupNode * scene,
											const std::vector<ray_t> & rays) {
	Context<value_t> context(rays);

	PROFILING_BEGIN(queryAction, "Query creation");

	typedef std::vector<IntersectionQuery<value_t>> queries_t;
	queries_t queries;
	queries.reserve(rays.size());
	for(std::size_t i = 0; i < rays.size(); ++i) {
		queries.emplace_back(context.slopes[i], context.results[i]);
	}
	
	PROFILING_END(queryAction);

	// Check if a triangle tree already exists.
	if(!hasSolidGeoTree(scene)) {
		PROFILING_BEGIN(treeAction, "Build geometry tree");
		storeSolidGeoTree(scene, buildSolidGeoTree(scene));
		PROFILING_END(treeAction);
	}
	const auto & tree = getSolidGeoTree(scene);

	visit(tree, context, queries);

	return context.results;
}

template<typename value_t>
typename RayCaster<value_t>::intersection_packet_t RayCaster<value_t>::castRays(
											GeometryNode * geoNode,
											const std::vector<ray_t> & rays) {
	// Search for a parent node that already has a tree.
	GroupNode * parent = geoNode->getParent();
	while(parent->hasParent() && !hasSolidGeoTree(parent)) {
		parent = parent->getParent();
	}

	Context<value_t> context(rays);

	PROFILING_BEGIN(queryAction, "Query creation");

	typedef std::vector<IntersectionQuery<value_t>> queries_t;
	queries_t queries;
	queries.reserve(rays.size());
	for(std::size_t i = 0; i < rays.size(); ++i) {
		queries.emplace_back(context.slopes[i], context.results[i]);
	}
	
	PROFILING_END(queryAction);

	// Check if a triangle tree already exists.
	if(!hasSolidGeoTree(parent)) {
		PROFILING_BEGIN(treeAction, "Build geometry tree");
		storeSolidGeoTree(parent, buildSolidGeoTree(parent));
		PROFILING_END(treeAction);
	}
	const auto & tree = getSolidGeoTree(parent);

	// Intersect the tree with the GeometryNode's bounding box.
	visit(tree, context, queries, geoNode->getWorldBB());

	return context.results;
}

// Instantiate the template with float
template class RayCaster<float>;

}
}

#endif /* MINSG_EXT_RAYCASTING */
