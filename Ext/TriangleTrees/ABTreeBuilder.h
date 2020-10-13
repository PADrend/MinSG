/*
	This file is part of the MinSG library extension TriangleTrees.
	Copyright (C) 2009-2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_TRIANGLETREES

#ifndef ABTREEBUILDER_H_
#define ABTREEBUILDER_H_

#include "TriangleTreeBuilder.h"
#include <cstddef>

namespace Rendering {
class Mesh;
}
namespace MinSG {
namespace TriangleTrees {
class TriangleTree;

/**
 * Class that creates a kDTree from a MinSG scene given by its root
 * node. It extracts the triangles needed by the kDTree from meshes of
 * the GeometryNodes inside the scene.
 *
 * @author Benjamin Eikel
 * @date 2009-06-29
 */
class ABTreeBuilder : public Builder {
	public:
		explicit ABTreeBuilder(std::size_t _trianglesPerNode, float _allowedBBEnlargement) :
				Builder(), trianglesPerNode(_trianglesPerNode), allowedBBEnlargement(_allowedBBEnlargement) {
		}

		/**
		 * Create an ABTree root by extracting geometry from @a mesh.
		 *
		 * @param mesh Mesh containing geometry.
		 * @return Root node of constructed ABTree.
		 * @see ABTree::ABTree()
		 */
		MINSGAPI TriangleTree * buildTriangleTree(Rendering::Mesh * mesh) override;

	private:
		//! Maximum number of triangles in a node. If this number is exceeded, the node will be split.
		std::size_t trianglesPerNode;

		//! Share of extent that the bounding box might be enlarged to pick up triangles, which cut the splitting plane.
		float allowedBBEnlargement;
};

}
}

#endif /* ABTREEBUILDER_H_ */

#endif /* MINSG_EXT_TRIANGLETREES */
