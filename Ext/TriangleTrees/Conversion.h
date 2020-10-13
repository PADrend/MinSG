/*
	This file is part of the MinSG library extension TriangleTrees.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_TRIANGLETREES

#ifndef MINSG_EXT_TRIANGLETREES_CONVERSION_H
#define MINSG_EXT_TRIANGLETREES_CONVERSION_H

#include <Rendering/Mesh/VertexDescription.h>

#include <utility>
#include <vector>

namespace Geometry {
template<typename value_t> class _Box;
typedef _Box<float> Box_f;
template<typename T_> class _Vec3;
template<typename T_> class Triangle;
typedef Triangle<_Vec3<float>> Triangle_f;
}
namespace MinSG {
class GeometryNode;
namespace TriangleTrees {

template<class bound_t, class triangle_t> class SolidTree;
//! Tree in three dimensions using @c float values
typedef SolidTree<Geometry::Box_f, Geometry::Triangle_f> SolidTree_3f;
/**
 * Tree in three dimensions using @c float values. It additionally stores
 * a pointer per triangle to the GeometryNode the triangle belongs to.
 */
typedef SolidTree<Geometry::Box_f, 
				  std::pair<Geometry::Triangle_f, 
							GeometryNode *>> SolidTree_3f_GeometryNode;
class TriangleTree;

/**
 * Convert the data structure stored in a TriangleTree into a SolidTree.
 * 
 * @param treeNode Input of the conversion: A TriangleTree referencing
 * triangles stored in a mesh
 * @return Output of the conversion: A SolidTree directly storing the triangles
 */
MINSGAPI SolidTree_3f convertTree(const TriangleTree * treeNode);

/**
 * Convert the data structure stored in a TriangleTree into a SolidTree.
 * Additionally, convert GeometryNode identifiers stored in the vertex data to
 * pointers to GeometryNodes.
 * 
 * @param treeNode Input of the conversion: A TriangleTree referencing
 * triangles stored in a mesh
 * @param idAttr Vertex attribute that stores the GeometryNode identifiers
 * @param idLookup Mapping from GeometryNode identifiers (index positions) to
 * pointers to GeometryNodes
 * @return Output of the conversion: A SolidTree directly storing the triangles
 * and pointers to GeometryNodes
 */
MINSGAPI SolidTree_3f_GeometryNode convertTree(const TriangleTree * treeNode,
									  const Rendering::VertexAttribute & idAttr,
									  const std::vector<GeometryNode *> & idLookup);

}
}

#endif /* MINSG_EXT_TRIANGLETREES_CONVERSION_H */

#endif /* MINSG_EXT_TRIANGLETREES */
