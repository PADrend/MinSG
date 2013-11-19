/*
	This file is part of the MinSG library extension
	AdaptiveGlobalVisibilitySampling.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_ADAPTIVEGLOBALVISIBILITYSAMPLING

#ifndef MINSG_AGVS_MUTATIONCANDIDATE_H
#define MINSG_AGVS_MUTATIONCANDIDATE_H

#include <cstdint>
#include <utility>

namespace Geometry {
template<typename T_> class _Vec3;
}
namespace MinSG {
class Node;
class GeometryNode;
namespace AGVS {

/**
 * Structure to store a single mutation candidate.
 * 
 * @tparam value_t Either @c float or @c double
 */
template<typename value_t>
class MutationCandidate {
	public:
		typedef Geometry::_Vec3<value_t> vec3_t;

		//! Segment origin (called s_0)
		vec3_t origin;

		//! Segment termination point (called s_t)
		vec3_t termination; 

		//! View cell at origin, or object at backward hit point
		Node * originObject;

		//! Object at forward hit point
		GeometryNode * terminationObject;

		//! Number of mutations generated from this candidate
		uint32_t mutationCount;

		MutationCandidate(vec3_t p_origin,
						  Node * p_originObject,
						  vec3_t p_termination, 
						  GeometryNode * p_terminationObject) :
			origin(std::move(p_origin)),
			termination(std::move(p_termination)),
			originObject(p_originObject),
			terminationObject(p_terminationObject),
			mutationCount(0) {
		}
};

}
}

#endif /* MINSG_AGVS_MUTATIONCANDIDATE_H */

#endif /* MINSG_EXT_ADAPTIVEGLOBALVISIBILITYSAMPLING */
