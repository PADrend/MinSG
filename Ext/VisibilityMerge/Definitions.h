/*
	This file is part of the MinSG library extension VisibilityMerge.
	Copyright (C) 2009-2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_VISIBILITYMERGE

#ifndef DEFINITIONS_H_
#define DEFINITIONS_H_

#include "../../Core/Nodes/GeometryNode.h"
#include <cstdint>
#include <set>
#include <unordered_map>

namespace Util {
class GenericAttributeList;
}
namespace MinSG {
// Forward declarations.
class GeometryNode;
class ValuatedRegionNode;

namespace VisibilitySubdivision {
//! Type of objects.
typedef GeometryNode object_t;
//! Type of cells.
typedef ValuatedRegionNode cell_t;
//! Pointer to an object.
typedef object_t * object_ptr;
//! Pointer to a visibility cell.
typedef cell_t * cell_ptr;
//! Pointer to a visibility list.
typedef Util::GenericAttributeList * list_ptr;

//! Costs of an object which is the same as the number of triangles.
typedef uint_fast32_t costs_t;
//! Costs per volume
typedef float costs_volume_t;

//! Set of unsorted visibility cells.
typedef std::set<cell_ptr> cell_set_t;

//! Structure used to sort objects by their triangle count.
struct ObjectCompare {
	bool operator()(const object_ptr & a, const object_ptr & b) const {
		const auto countA = a->getTriangleCount();
		const auto countB = b->getTriangleCount();
		if (countA == countB) {
			return a < b;
		}
		return countA < countB;
	}
};
//! Set of sorted objects.
typedef std::set<object_ptr, ObjectCompare> sorted_object_set_t;

//! Mapping from an object to the set of cells which see the object.
typedef std::unordered_map<object_ptr, cell_set_t> reverse_map_t;

//! Mapping from an object to the set of cells which see the object.
typedef std::unordered_map<list_ptr, cell_set_t> visibility_sharer_map_t;
}
}

#endif /* DEFINITIONS_H_ */
#endif /* MINSG_EXT_VISIBILITYMERGE */
