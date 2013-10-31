/*
	This file is part of the MinSG library extension RayCasting.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_RAYCASTING

#ifndef MINSG_EXT_TRIANGLETREES
#error "MINSG_EXT_RAYCASTING requires MINSG_EXT_TRIANGLETREES."
#endif /* MINSG_EXT_TRIANGLETREES */

#ifndef MINSG_RAYCASTING_RAYCASTER_H
#define MINSG_RAYCASTING_RAYCASTER_H

#include <vector>

namespace Geometry {
template<typename T_> class _Vec3;
template<typename vec_t> class _Ray;
}
namespace MinSG {
class GeometryNode;
class GroupNode;
namespace RayCasting {

//! Class to perform ray casting
template<typename value_t>
class RayCaster {
	public:
		typedef std::pair<GeometryNode *, value_t> intersection_t;
		typedef std::vector<intersection_t> intersection_packet_t;

		typedef Geometry::_Vec3<value_t> vec_t;
		typedef Geometry::_Ray<vec_t> ray_t;

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
		static intersection_packet_t castRays(GroupNode * scene,
											  const std::vector<ray_t> & rays);

		/**
		 * Cast a packet of rays against a single object and check if the object
		 * is hit.
		 * 
		 * @param scene Root node of the scene that will be used for casting
		 * @param rays Array of rays, given in the world coordinate system
		 * @return Array of intersection results. Each result contains the
		 * object if it is hit by the ray, or @c nullptr if it is not hit, as
		 * first entry. The intersection value for the point where the object is
		 * hit is stored in the second entry of the result.
		 */
		static intersection_packet_t castRays(GeometryNode * geoNode,
											  const std::vector<ray_t> & rays);
};

}
}

#endif /* MINSG_RAYCASTING_RAYCASTER_H */

#endif /* MINSG_EXT_RAYCASTING */
