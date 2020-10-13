/*
	This file is part of the MinSG library extension VisibilityMerge.
	Copyright (C) 2009-2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_VISIBILITYMERGE

#ifndef VISIBILITYMERGE_H_
#define VISIBILITYMERGE_H_

#include "Definitions.h"
#include "Helper.h"

namespace MinSG {
class ListNode;
namespace SceneManagement {
class SceneManager;
}
//! @ingroup ext
namespace VisibilityMerge {
/**
 * Class to merge geometric objects and visibility regions based on
 * visibility information to reduce memory usage.
 *
 * @author Benjamin Eikel
 * @date 2009-08-14
 */
class VisibilityMerge {
	public:
		/**
		 * Run the visibility merge on the given visibility region
		 * hierarchy.
		 *
		 * @param mgr Scene manager used for registration of nodes.
		 * @param root Root node of a visibility region hierarchy.
		 * @param wsSize Size of the working subset.
		 * @param b_D Minimum number of triangles for objects.
		 * @param b_SPO Maximum number of objects
		 * @param b_SPZ Maximum number of cells
		 * @return Combination of new root node of visibility
		 * subdivision and new root node of scene graph.
		 */
		MINSGAPI static std::pair<ValuatedRegionNode *, ListNode *> run(SceneManagement::SceneManager * mgr,
																VisibilitySubdivision::cell_ptr root, const size_t wsSize,
																const VisibilitySubdivision::costs_t b_D,
																const VisibilitySubdivision::costs_t b_SPO,
																const VisibilitySubdivision::costs_t b_SPZ);

	private:
		/**
		 * Use the visibility vectors contained in the nodes in
		 * @a cells to fill the @a reverseMap map. Furthermore collect
		 * the objects in @a objects.
		 *
		 * @param cells Visibility cells.
		 * @param objects Object set which will be filled.
		 * @param reverseMap Mapping to store reverse visibility
		 * information.
		 */
		MINSGAPI static void createReverseMapping(const VisibilitySubdivision::cell_set_t & cells,
										VisibilitySubdivision::sorted_object_set_t & objects,
										VisibilitySubdivision::reverse_map_t & reverseMap);

		/**
		 * Calculate the costs which will be induced when the two given
		 * objects would be merged. The calculation includes the number
		 * of triangles of the objects and the volume of the visibility
		 * cells.
		 *
		 * @param o_i First object
		 * @param o_j Second object
		 * @param reverseMap Mapping form objects to visibility cells
		 * @return Costs induced when the two object are merged
		 */
		MINSGAPI static VisibilitySubdivision::costs_volume_t getMergeCosts(VisibilitySubdivision::object_ptr o_i,
																	VisibilitySubdivision::object_ptr o_j,
																	VisibilitySubdivision::reverse_map_t & reverseMap);

		/**
		 * Reduce object space by merging objects.
		 *
		 * @param objectSpace Object space which will be modified
		 * @param reverseMap Mapping form objects to visibility cells
		 * @param wsSize Working set size. Only the first @a wss
		 * objects from input will be used. The rest is copied directly
		 * to the output.
		 * @param b_D Minimum object size to achieve
		 * @param b_SPO Maximum number of objects
		 * @return Number of merges that were done
		 */
		MINSGAPI static size_t objectSpaceReduce(VisibilitySubdivision::sorted_object_set_t & objectSpace,
										VisibilitySubdivision::reverse_map_t & reverseMap, const size_t wsSize,
										const VisibilitySubdivision::costs_t b_D,
										const VisibilitySubdivision::costs_t b_SPO);

		/**
		 * Merge two objects and update the references to these
		 * objects.
		 *
		 * @param o_i First object
		 * @param o_j Second object
		 * @param reverseMap Mapping form objects to visibility cells
		 * @return Merged object
		 */
		MINSGAPI static VisibilitySubdivision::object_ptr mergeObjects(VisibilitySubdivision::object_ptr o_i, VisibilitySubdivision::object_ptr o_j,
										VisibilitySubdivision::reverse_map_t & reverseMap);

		/**
		 * Calculate the costs which will be induced when the two given
		 * lists of visibility vectors would be merged. The calculation
		 * includes the number of triangles of the visible objects, the
		 * volumes of all the cells using these lists and the number of
		 * references which can be saved.
		 *
		 * @param l_i First list
		 * @param l_j Second list
		 * @return Costs induced when the two lists are merged
		 */
		MINSGAPI static VisibilitySubdivision::costs_volume_t getMergeScoreLists(
												const VisibilitySubdivision::visibility_sharer_map_t::const_iterator & l_i,
												const VisibilitySubdivision::visibility_sharer_map_t::const_iterator & sl_j);

		/**
		 * Reduce view space by merging cells.
		 * This merge routine considers any two cells.
		 * It keeps the cell volumes and only merge the visibility vectors.
		 *
		 * @param sharer Mapping from lists of visibility vectors to
		 * the cells which use this list.
		 * @param wsSize Working set size. Only the first @a wss
		 * lists will be used.
		 * @param b_SPZ Maximum number of cells
		 * @return Number of merges that were done
		 */
		MINSGAPI static size_t viewSpaceReduceGlobal(VisibilitySubdivision::visibility_sharer_map_t & sharer, const size_t wsSize,
											const VisibilitySubdivision::costs_t b_SPZ);

		/**
		 * Merge only the visibility vectors of the visibility list and
		 * assign the new list to the cells using these vectors.
		 *
		 * @param sharer Map holding information about shared
		 * visibility vectors
		 * @param l_i First list
		 * @param l_j Second list
		 */
		MINSGAPI static void mergeVisibility(VisibilitySubdivision::visibility_sharer_map_t & sharer,
									VisibilitySubdivision::list_ptr l_i,
									VisibilitySubdivision::list_ptr l_j);
};
}
}

#endif /* VISIBILITYMERGE_H_ */
#endif /* MINSG_EXT_VISIBILITYMERGE */
