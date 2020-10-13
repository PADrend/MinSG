/*
	This file is part of the MinSG library extension PathTracing.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2017 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_PATHTRACING

#ifndef MINSG_EXT_TRIANGLETREES
#error "MINSG_EXT_PATHTRACING requires MINSG_EXT_TRIANGLETREES."
#endif /* MINSG_EXT_TRIANGLETREES */

#ifndef MINSG_EXT_PATHTRACING_RAYCASTER_H_
#define MINSG_EXT_PATHTRACING_RAYCASTER_H_

#include "TreeBuilder.h"

#include <vector>
#include <tuple>

namespace Geometry {
template<typename T_> class _Vec3;
template<typename T_> class _Vec2;
template<typename vec_t> class _Ray;
template<typename T_> class _Box;
}
namespace MinSG {
class GeometryNode;
class GroupNode;
namespace PathTracing {
class ExtTriangle;

//! Class to perform ray casting
template<typename value_t>
class RayCaster {
	public:
		// (distance, u, v, triangle)			
		typedef std::tuple<value_t, value_t, value_t, ExtTriangle> intersection_t;
		typedef std::vector<intersection_t> intersection_packet_t;

		typedef Geometry::_Vec3<value_t> vec_t;
		typedef Geometry::_Ray<vec_t> ray_t;
		typedef Geometry::_Box<value_t> box_t;	

		/**
		 * Cast a packet of rays against a scene and return the first objects
		 * that are hit together with the intersection distance.
		 * 
		 * @param scene Root node of the scene that will be used for casting
		 * @param rays Array of rays, given in the world coordinate system
		 * @return Array of intersection results. Each result contains the first
		 * object that is hit by the ray, or @c nullptr if no object is hit, as
		 * first entry. The intersection value for the point where an object is
		 * hit is stored in the second entry of the result.
		 */
		MINSGAPI static intersection_packet_t castRays(const SolidTree_ExtTriangle& tree,
											  const std::vector<ray_t> & rays, const box_t& bounds);

};

}
}

#endif /* MINSG_EXT_PATHTRACING_RAYCASTER_H_ */

#endif /* MINSG_EXT_PATHTRACING */
