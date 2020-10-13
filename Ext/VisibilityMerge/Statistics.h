/*
	This file is part of the MinSG library extension VisibilityMerge.
	Copyright (C) 2009-2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_VISIBILITYMERGE

#ifndef STATISTICS_H_
#define STATISTICS_H_

#include "Definitions.h"
#include "Helper.h"
#include <fstream>
#include <string>

namespace MinSG {
class ValuatedRegionNode;

namespace VisibilityMerge {

class Statistics {
	public:
		/**
		 * Constructor the statistics object by opening the output
		 * file.
		 *
		 * @param fileName Store the tab separated data into the file
		 * given by this parameter.
		 */
		MINSGAPI Statistics(const std::string & fileName);

		/**
		 * Destructor closes the output file.
		 */
		MINSGAPI ~Statistics();

		/**
		 * Return the value of the current pass counter.
		 *
		 * @return Number of the current pass.
		 */
		unsigned int getPass() const {
			return pass;
		}


		/**
		 * Generate and output statistics for a whole run. Generates a
		 * file with only one data row.
		 * In addition to the parameters output a time stamp.
		 *
		 * @param fileName Store the tab separated data into the file
		 * given by this parameter.
		 * @param timeStamp Time stamp in seconds.
		 * @param description Text description of the run.
		 * @param wsSize Size of the working set.
		 * @param trianglesMinLimit Minimum object triangle limit of run.
		 * @param objectCountLimit Maximum number of objects of run.
		 * @param cellCountLimit Maximum number of cells of run.
		 * @param passBoundary First pass number of the view space
		 * reduction. This number separates the object space reduction
		 * from the view space reduction.
		 */
		MINSGAPI static void perRun(const std::string & fileName, const std::string & timeStamp, const std::string & description, size_t wsSize,
							VisibilitySubdivision::costs_t trianglesMinLimit, VisibilitySubdivision::costs_t objectCountLimit,
							VisibilitySubdivision::costs_t cellCountLimit, unsigned int passBoundary);

		/**
		 * Generate and output statistics for a single pass of a run.
		 *
		 * @param time Duration of pass
		 * @param numMerges Number of merges done during pass
		 */
		MINSGAPI void perPass(double duration, size_t numMerges);

		/**
		 * Generate and output statistics for individual visibility
		 * list. This includes the number of objects and the number of
		 * triangles which are visible from a visibility list.
		 *
		 * @param lists Visibility lists of a visibility subdivision
		 */
		MINSGAPI void perList(const VisibilitySubdivision::visibility_sharer_map_t & lists);

		/**
		 * Generate and output statistics for individual visibility
		 * cells. This includes the number of objects and the number of
		 * triangles which are visible from a visibility cell.
		 *
		 * @param cells Cells of a visibility subdivision
		 */
		MINSGAPI void perCell(const VisibilitySubdivision::cell_set_t & cells);

		/**
		 * Generate and output statistics for individual objects. This
		 * includes the number of triangles and the volume of an
		 * object.
		 *
		 * @param objects Object space
		 */
		MINSGAPI void perObject(const VisibilitySubdivision::sorted_object_set_t & objects);

	private:
		//! Output stream to paste data to the file.
		std::ofstream out;

		//! Flag to remember if per Pass statistics were output.
		bool perPassOutput;

		//! Flag to remember if per cell statistics were output.
		bool perListOutput;

		//! Flag to remember if per cell statistics were output.
		bool perCellOutput;

		//! Flag to remember if per object statistics were output.
		bool perObjectOutput;

		//! Counter for calls to identify data sets.
		unsigned int pass;
};

}

}

#endif /* STATISTICS_H_ */
#endif /* MINSG_EXT_VISIBILITYMERGE */
