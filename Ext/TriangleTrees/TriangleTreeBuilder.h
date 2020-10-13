/*
	This file is part of the MinSG library extension TriangleTrees.
	Copyright (C) 2009-2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_TRIANGLETREES

#ifndef TRIANGLETREEBUILDER_H_
#define TRIANGLETREEBUILDER_H_

#include <vector>

namespace Rendering {
class Mesh;
class VertexDescription;
}
namespace MinSG {
class GeometryNode;
class Node;
namespace TriangleTrees {
class TriangleTree;

/**
 * Public interface for building trees on triangle level.
 *
 * @author Benjamin Eikel
 * @date 2011-07-22
 */
class Builder {
	public:
		/**
		 * Collect all meshes from the GeometryNodes and store them together in a single mesh.
		 * The global transformation of the GeometryNodes is respected.
		 *
		 * @param geoNodes Container with GeometryNodes.
		 * @return A new mesh containing all the mesh data.
		 * @throw Exception describing what went wrong.
		 */
		MINSGAPI static Rendering::Mesh * mergeGeometry(const std::vector<GeometryNode *> & geoNodes);

		/**
		 * Take the triangles from the given mesh and organize them in a hierarchical spatial data structure.
		 *
		 * @param mesh Container holding the triangles.
		 * @param builder Builder specifying the way in which the tree will be built.
		 * @return Root node of the tree.
		 */
		MINSGAPI static Node * buildMinSGTree(Rendering::Mesh * mesh, Builder & builder);

		/**
		 * Take the triangles from the given mesh and organize them in the way that is defined by this builder.
		 *
		 * @param mesh Container holding the triangles.
		 * @return Root node of the tree.
		 */
		virtual TriangleTree * buildTriangleTree(Rendering::Mesh * mesh) = 0;

	protected:
		virtual ~Builder() {
		}

		/**
		 * Convert TriangleTree nodes into MinSG nodes.
		 * Inner nodes will be converted to ListNodes.
		 * Leaves will be converted to GeometryNodes.
		 *
		 * @param treeNode Root node of the tree to convert.
		 * @param vertexDesc Vertex description for the mesh data stored in the tree.
		 * @return Converted MinSG node.
		 */
		MINSGAPI static Node * convert(const TriangleTree * treeNode, const Rendering::VertexDescription & vertexDesc);
};

}
}

#endif /* TRIANGLETREEBUILDER_H_ */

#endif /* MINSG_EXT_TRIANGLETREES */
