/*
	This file is part of the MinSG library extension TriangleTrees.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_TRIANGLETREES

#ifndef MINSG_EXT_TRIANGLETREES_SOLIDTREE_H
#define MINSG_EXT_TRIANGLETREES_SOLIDTREE_H

#include <type_traits>
#include <vector>

namespace MinSG {
namespace TriangleTrees {

/**
 * @brief Tree directly storing triangles only
 * 
 * In contrast to the TriangleTree, the triangles are stored directly in the
 * tree and all references to the original mesh are dropped. Only the positions
 * of the vertices are stored without further information that might be
 * available in the original mesh.
 * The tree is implemented in a way that it is meant to build the tree once and
 * never modify it.
 *
 * @tparam bound_t Type used to describe the bounds of a tree node
 * @tparam triangle_t Type of triangles stored in the tree
 * @author Benjamin Eikel
 * @date 2013-03-19
 */
template<class bound_t, class triangle_t>
class SolidTree {
	public:
		typedef std::vector<SolidTree> children_t;
		typedef std::vector<triangle_t> triangles_t;

		//! Create a new, empty tree node (no children, no triangles).
		SolidTree() :
			bound(),
			children(),
			triangles() {
		}

		//! Create a new tree node storing the given data.
		template<class other_bound_t,
				 class other_children_t,
				 class other_triangles_t,
				 typename = typename std::enable_if<
					 std::is_convertible<other_bound_t, bound_t>::value &&
					 std::is_convertible<other_children_t, children_t>::value &&
					 std::is_convertible<other_triangles_t, triangles_t>::value
				 >::type>
		SolidTree(other_bound_t && newBound,
				  other_children_t && newChildren,
				  other_triangles_t && newTriangles) :
			bound(std::forward<other_bound_t>(newBound)),
			children(std::forward<other_children_t>(newChildren)),
			triangles(std::forward<other_triangles_t>(newTriangles)) {
		}

		/**
		 * Return the bound of this node.
		 *
		 * @return Bound
		 */
		const bound_t & getBound() const {
			return bound;
		}

		/**
		 * Tell if the node is a leaf node.
		 *
		 * @return @c true if the node is a leaf and @c false if it is an
		 * inner node.
		 */
		bool isLeaf() const {
			return children.empty();
		}

		/**
		 * Access the child nodes.
		 *
		 * @return Array of children
		 */
		const children_t & getChildren() const {
			return children;
		}

		/**
		 * Set new child nodes.
		 *
		 * @param newChildren Array of children
		 * @note The bounds of the children are not checked to be inside the
		 * bounds of this node.
		 */
		void setChildren(const children_t & newChildren) {
			children = newChildren;
		}

		/**
		 * Access the triangles stored in this node.
		 *
		 * @return Array of triangles
		 */
		const triangles_t & getTriangles() const {
			return triangles;
		}

		/**
		 * Store triangles in this node.
		 *
		 * @param newTriangles Array of triangles
		 * @note The triangles are not checked to be inside the bounds of this
		 * node.
		 */
		void setTriangles(const triangles_t & newTriangles) {
			triangles = newTriangles;
		}

	private:
		//! Geometric bound of this node
		bound_t bound;

		//! Array of child nodes
		children_t children;

		//! Array of triangles
		triangles_t triangles;
};

}
}

#endif /* MINSG_EXT_TRIANGLETREES_SOLIDTREE_H */

#endif /* MINSG_EXT_TRIANGLETREES */
