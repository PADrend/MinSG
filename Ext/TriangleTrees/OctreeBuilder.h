/*
	This file is part of the MinSG library extension TriangleTrees.
	Copyright (C) 2009-2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_TRIANGLETREES

#ifndef OCTREEBUILDER_H_
#define OCTREEBUILDER_H_

#include "TriangleTreeBuilder.h"
#include <cstddef>

namespace Rendering {
class Mesh;
}
namespace MinSG {
namespace TriangleTrees {
class TriangleTree;

/**
 * Class that creates an octree, which can be a loose octree on request.
 *
 * @author Benjamin Eikel
 * @date 2011-07-26
 */
class OctreeBuilder : public Builder {
	public:
		explicit OctreeBuilder(std::size_t _trianglesPerNode, float _looseFactor) :
				Builder(), trianglesPerNode(_trianglesPerNode), looseFactor(_looseFactor) {
		}

		/**
		 * Create an octree root by extracting geometry from @a mesh.
		 *
		 * @param mesh Mesh containing geometry.
		 * @return Root node of constructed octree.
		 * @see Octree::Octree()
		 */
		MINSGAPI TriangleTree * buildTriangleTree(Rendering::Mesh * mesh) override;

	private:
		//! Maximum number of triangles in a node. If this number is exceeded, the node will be split.
		std::size_t trianglesPerNode;

		//! Factor by which the bounding boxes will be enlarged.
		float looseFactor;
};

}
}

#endif /* OCTREEBUILDER_H_ */

#endif /* MINSG_EXT_TRIANGLETREES */
