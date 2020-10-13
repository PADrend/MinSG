/*
	This file is part of the MinSG library extension PathTracing.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2017 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_PATHTRACING

#ifndef MINSG_EXT_PATHTRACING_TREEBUILDER_H
#define MINSG_EXT_PATHTRACING_TREEBUILDER_H

#include <Rendering/Mesh/VertexDescription.h>

#include <utility>
#include <vector>
#include <memory>

namespace Geometry {
template<typename value_t> class _Box;
typedef _Box<float> Box_f;
template<typename T_> class _Vec3;
template<typename T_> class Triangle;
typedef Triangle<_Vec3<float>> Triangle_f;
}
namespace Rendering {
class Mesh;
}
namespace MinSG {
class GeometryNode;
class GroupNode;
namespace TriangleTrees {
template<class bound_t, class triangle_t> class SolidTree;
class TriangleTree;
}

namespace PathTracing {
	
class Material;
class ExtTriangle;
typedef TriangleTrees::SolidTree<Geometry::Box_f, ExtTriangle> SolidTree_ExtTriangle;


MINSGAPI SolidTree_ExtTriangle buildSolidExtTree(GroupNode * scene, std::vector<std::unique_ptr<Material>>& materialLibrary);

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
MINSGAPI SolidTree_ExtTriangle convertTree(const TriangleTrees::TriangleTree * treeNode,
										Rendering::Mesh* mesh,
									  const Rendering::VertexAttribute & idAttr,
									  const std::vector<GeometryNode *> & idLookup, 
										std::vector<std::unique_ptr<Material>>& materialLibrary);

}
}

#endif /* MINSG_EXT_PATHTRACING_TREEBUILDER_H */
#endif /* MINSG_EXT_PATHTRACING */
