/*
	This file is part of the MinSG library extension TwinPartitions.
	Copyright (C) 2010-2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_TWIN_PARTITIONS

#ifndef PARTITIONSDATA_H_
#define PARTITIONSDATA_H_

#include <Geometry/Box.h>
#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Texture/Texture.h>
#include <Util/References.h>
#include <cstdint>
#include <string>
#include <vector>

namespace MinSG {
namespace TwinPartitions {

struct PartitionsData {
		/**
		 * Storage for the whole object space.
		 * The object space is the partition of the triangle soup into geometric objects.
		 * One object is represented by a mesh.
		 */
		std::vector<Util::Reference<Rendering::Mesh> > objects;

		/**
		 * A visible set contains pairs of cost-benefit ratios and indices into the @a objects array.
		 * The cost-benefit ratio is the quotient of the number of triangles to the number of visible pixels of the referenced object.
		 * The array is sorted in ascending order.
		 */
		typedef std::vector<std::pair<float, uint32_t> > visible_set_t;

		/**
		 * Storage for the visible sets of the view space.
		 */
		std::vector<visible_set_t> visibleSets;

		/**
		 * Storage for the depth meshes of the cells.
		 */
		std::vector<Util::Reference<Rendering::Mesh> > depthMeshes;

		/**
		 * Storage for the texture file names of the cells.
		 */
		std::vector<std::string> textureFiles;

		/**
		 * Structure representing one cell of the view space.
		 */
		struct cell_t {
			//! Bounding box of the cell.
			Geometry::Box bounds;
			//! Index into the @a visibleSets array. The indexed visible set contains the references to geometric objects that are visible from the cell.
			uint32_t visibleSetId;
			//! Indices into the @a depthMeshes and @a textureFiles array. The indexed textured depth meshes represent the surroundings of the cell.
			std::vector<uint32_t> surroundingIds;

			cell_t(Geometry::Box boundingBox, uint32_t visibleSet, std::vector<uint32_t> surroundings) :
				bounds(std::move(boundingBox)), visibleSetId(visibleSet), surroundingIds(std::move(surroundings)) {
			}
		};

		/**
		 * Storage for the cells of the view space.
		 */
		std::vector<cell_t> cells;
};

}
}

#endif /* PARTITIONSDATA_H_ */
#endif /* MINSG_EXT_TWIN_PARTITIONS */
