/*
	This file is part of the MinSG library extension TriangleTrees.
	Copyright (C) 2009-2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_TRIANGLETREES

#ifndef TRIANGLETREE_H_
#define TRIANGLETREE_H_

#include <Geometry/Box.h>
#include <cstdint>
#include <memory>
#include <vector>

namespace Rendering {
class Mesh;
namespace MeshUtils {
class LocalMeshDataHolder;
}
}
namespace Util {
class AttributeProvider;
}
namespace MinSG {
//! @ingroup ext
namespace TriangleTrees {
class TriangleAccessor;

/**
 * Abstract base class for trees working with triangles.
 *
 * @author Benjamin Eikel
 * @date 2011-07-29
 */
class TriangleTree {
	public:
		/**
		 * Create a TriangleTree root node and get the bounding box from the given mesh.
		 *
		 * @param mesh Mesh containing the triangles.
		 */
		MINSGAPI explicit TriangleTree(Rendering::Mesh * mesh);

		//! Empty destructor.
		MINSGAPI virtual ~TriangleTree();

		/**
		 * Return the bounding box of this node.
		 *
		 * @return Axis-aligned bounding box.
		 */
		const Geometry::Box & getBound() const {
			return bound;
		}

		/**
		 * Return the level in the tree. The root node has level zero.
		 *
		 * @return Level of the node.
		 */
		uint8_t getLevel() const {
			return level;
		}

		/**
		 * Tell if the node is a leaf node.
		 *
		 * @return @c true if the node is a leaf and @c false if it is an inner node.
		 */
		virtual bool isLeaf() const = 0;

		/**
		 * Retrieve to the child nodes.
		 *
		 * @return Pointers to children.
		 */
		virtual std::vector<const TriangleTree *> getChildren() const = 0;

		/**
		 * Return one triangle.
		 *
		 * @param index Index of triangle stored in this node.
		 * @return Constant accessor for the triangle.
		 */
		virtual const TriangleAccessor & getTriangle(uint32_t index) const = 0;

		/**
		 * Return the number of triangles that are stored in this tree node.
		 *
		 * @return Triangle count
		 */
		virtual uint32_t getTriangleCount() const = 0;

		/**
		 * Return the overall number of triangles stored in the subtree.
		 * If this is an inner node, return the number of triangles stored in this node plus the sum of the children.
		 * Otherwise return the number of triangles stored in this leaf.
		 *
		 * @return Triangle count
		 */
		MINSGAPI uint32_t countTriangles() const;

		/**
		 * Return the number of triangles that are stored in inner nodes of the subtree.
		 * If this is an inner node, return the number of triangles stored in this node and recurse into children.
		 * Otherwise return zero.
		 *
		 * @return Number of triangles that are stored in inner nodes.
		 */
		MINSGAPI uint32_t countInnerTriangles() const;

		/**
		 * Return the number of triangles that are outside the bounding box of the node they are stored in.
		 * If this is an inner node, return the number of triangles stored in this node and recurse into children.
		 * Otherwise count the number of triangles that are stored in this node and are outside the bounding box.
		 *
		 * @return Number of triangles outside the bounding box.
		 */
		MINSGAPI uint32_t countTrianglesOutside() const;

		/**
		 * Check if the triangle fits into the bounding box of this tree node.
		 *
		 * @param triangle Triangle to check.
		 * @return @c true if the triangle is inside this node, otherwise @c false.
		 */
		virtual bool contains(const TriangleAccessor & triangle) const = 0;

		/**
		 * Add attributes specific to this tree node to the given container.
		 *
		 * @param container Container for attributes.
		 */
		virtual void fetchAttributes(Util::AttributeProvider * container) const = 0;

	private:
		//! Holder to ensure that the mesh data stays valid.
		std::unique_ptr<Rendering::MeshUtils::LocalMeshDataHolder> meshHolder;
		
		//! Axis-aligned bounding box.
		Geometry::Box bound;

		//! Level in the tree. Root starts with zero.
		uint8_t level;

	protected:
		/**
		 * Create a new TriangleTree node and assign variables from the parent node.
		 *
		 * @param childBound Axis-aligned bounding box for the child.
		 * @param parent Parent node which is used to copy the parameters from.
		 */
		MINSGAPI explicit TriangleTree(Geometry::Box childBound, const TriangleTree & parent);

		/**
		 * Set the bounding box of this node.
		 *
		 * @param newBound Axis-aligned bounding box.
		 */
		void setBound(const Geometry::Box & newBound) {
			bound = newBound;
		}
};

}
}

#endif /* TRIANGLETREE_H_ */

#endif /* MINSG_EXT_TRIANGLETREES */
