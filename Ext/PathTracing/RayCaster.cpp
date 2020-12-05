/*
	This file is part of the MinSG library extension PathTracing.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2017 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_PATHTRACING

#include "RayCaster.h"
#include "TreeBuilder.h"
#include "ExtTriangle.h"
#include "Material.h"

#include "../TriangleTrees/SolidTree.h"
#include <Geometry/BoxHelper.h>
#include <Geometry/BoxIntersection.h>
#include <Geometry/Line.h>
#include <Geometry/LineTriangleIntersection.h>
#include <Geometry/RayBoxIntersection.h>
#include <Geometry/Triangle.h>
#include <Geometry/Vec2.h>
#include <Util/References.h>
#include <Util/StringIdentifier.h>
#include <Util/Utils.h>
#include <cstdint>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

namespace MinSG {
namespace PathTracing {
	
template<typename value_t>
class Context {
	public:
		typedef Geometry::_Vec3<value_t> vec3_t;
		typedef Geometry::_Ray<vec3_t> ray_t;
		typedef Geometry::Intersection::Slope<value_t> slope_t;

		const std::vector<slope_t> slopes;
		typename RayCaster<value_t>::intersection_packet_t results;

		Context(const std::vector<ray_t> & rays) :
			slopes(rays.cbegin(), rays.cend()),
			results(rays.size(), 
					std::make_tuple(std::numeric_limits<value_t>::max(), value_t(), value_t(), ExtTriangle())) {
		}
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
			return std::get<0>(result);
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
			std::get<0>(result) = distance;
		}
		void setObject(const ExtTriangle& object) const {
			std::get<3>(result) = object;
		}
		void setUV(value_t u, value_t v) const {
			std::get<1>(result) = u;
			std::get<2>(result) = v;
		}
};

template<typename value_t>
static void testTrianglesInNode(const SolidTree_ExtTriangle & node,
								const std::vector<IntersectionQuery<value_t>> & queries,
								Context<value_t> & context) {

	for(const auto & triangleData : node.getTriangles()) {
		const auto & triangle = triangleData.pos;
		value_t t, u, v;
		for(const auto & query : queries) {
			const auto & ray = query.getRay();
			using namespace Geometry::Intersection;
			const bool intersection = getLineTriangleIntersection(ray, triangle, t, u, v);
			if(intersection && !(t < 0) && t < query.getDistance()) {
				query.setDistance(t);
				query.setObject(triangleData);
				query.setUV(u, v);
			}
		}
	}
	
}

//! Tree traversal for ray casting
template<typename value_t>
static void visit(const SolidTree_ExtTriangle & node,
				  Context<value_t> & context,
				  const std::vector<IntersectionQuery<value_t>> & queries) {

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
static void visit(const SolidTree_ExtTriangle & node,
				  Context<value_t> & context,
				  const std::vector<IntersectionQuery<value_t>> & queries,
				  const Geometry::Box & testBox) {

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
											const SolidTree_ExtTriangle& tree,
											const std::vector<ray_t> & rays, 
											const box_t& bounds) {
	Context<value_t> context(rays);

	typedef std::vector<IntersectionQuery<value_t>> queries_t;
	queries_t queries;
	queries.reserve(rays.size());
	for(std::size_t i = 0; i < rays.size(); ++i) {
		queries.emplace_back(context.slopes[i], context.results[i]);
	}

	visit(tree, context, queries);

	return context.results;
}

// Instantiate the template with float
template class RayCaster<float>;

}
}

#endif /* MINSG_EXT_PATHTRACING */
