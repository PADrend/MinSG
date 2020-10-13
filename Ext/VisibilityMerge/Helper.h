/*
	This file is part of the MinSG library extension VisibilityMerge.
	Copyright (C) 2009-2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_VISIBILITYMERGE

#ifndef HELPER_H_
#define HELPER_H_

#include "Definitions.h"

namespace Geometry {
template<typename value_t> class _Box;
typedef _Box<float> Box;
}
namespace Util {
class GenericAttribute;
class GenericAttributeList;
}
namespace MinSG {
namespace VisibilitySubdivision {
class VisibilityVector;
}
namespace VisibilityMerge {

class Helper {
	public:
		/**
		 * Convenience function to extract the list of visibility
		 * vectors from a visibility region node.
		 *
		 * @param node Region which is used as source.
		 * @return A pointer to the extracted list or @c nullptr if the
		 * list could not be extracted.
		 */
		MINSGAPI static VisibilitySubdivision::list_ptr getVVList(const VisibilitySubdivision::cell_ptr node);

		/**
		 * Convenience function to extract the visibility vector from
		 * a GenericAttrbiute.
		 *
		 * @param attribute GenericAttribute which is used as source.
		 * @return Extracted visibility vector
		 */
		MINSGAPI static const VisibilitySubdivision::VisibilityVector & getVV(const Util::GenericAttribute * attribute);

		/**
		 * Convenience function to access the visibility vector stored in a
		 * GenericAttrbiute.
		 *
		 * @param attribute GenericAttribute that stores the data
		 * @return Reference to the visibility vector
		 */
		MINSGAPI static VisibilitySubdivision::VisibilityVector & getVV(Util::GenericAttribute * attribute);

		/**
		 * Go recursively through the tree given by @a root and extract
		 * all leaf cells.
		 *
		 * @param root Root node of visibility subdivision subtree.
		 * @return Leaf cells that were found.
		 */
		MINSGAPI static VisibilitySubdivision::cell_set_t collectVisibilityCells(VisibilitySubdivision::cell_ptr root);

		/**
		 * Calculate the overall runtime for a visibility list by
		 * calculating the sum of triangles of visible objects from
		 * that cell. The result is stored in an internal cache.
		 *
		 * @param list Visibility list
		 * @return Runtime costs
		 */
		MINSGAPI static VisibilitySubdivision::costs_t getRuntime(const VisibilitySubdivision::list_ptr cell);

		/**
		 * Invalidate the runtime of the given list in the internal
		 * cache. This function has to be called when a list was
		 * modified and the runtime might have been changed.
		 *
		 * @param list Visibility list
		 */
		MINSGAPI static void clearRuntime(const VisibilitySubdivision::list_ptr list);

		/**
		 * Calculate the overall runtime for a visibility cell by
		 * calculating the sum of triangles of visible objects from
		 * that cell. The result is stored in an internal cache.
		 *
		 * @param cell Visibility cell
		 * @return Runtime costs
		 */
		static VisibilitySubdivision::costs_t getRuntime(const VisibilitySubdivision::cell_ptr cell) {
			return getRuntime(getVVList(cell));
		}


		/**
		 * Invalidate the runtime of the given cell in the internal
		 * cache. This function has to be called when a cell was
		 * modified and the runtime might have been changed.
		 *
		 * @param cell Visibility cell
		 */
		static void clearRuntime(const VisibilitySubdivision::cell_ptr cell) {
			clearRuntime(getVVList(cell));
		}


		/**
		 * Accumulate the visibility vectors of a list of visibility
		 * vectors.
		 *
		 * @param list List of visibility vectors
		 * @return Visibility vector representing the maximum
		 * visibility of the list
		 */
		MINSGAPI static VisibilitySubdivision::VisibilityVector getMaximumVisibility(const VisibilitySubdivision::list_ptr list);

		/**
		 * Accumulate the visibility vectors of a visibility cell.
		 *
		 * @param cell Visibility cell
		 * @return Visibility vector representing the maximum
		 * visibility of the cell
		 */
		MINSGAPI static VisibilitySubdivision::VisibilityVector getMaximumVisibility(const VisibilitySubdivision::cell_ptr cell);

	private:
		//! Cache for the calculations of @a getRuntime.
		MINSGAPI static std::unordered_map<VisibilitySubdivision::list_ptr, VisibilitySubdivision::costs_t> runtimeCache;
};
}
}

#endif /* HELPER_H_ */
#endif /* MINSG_EXT_VISIBILITYMERGE */
