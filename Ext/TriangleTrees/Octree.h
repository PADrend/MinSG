/*
	This file is part of the MinSG library extension TriangleTrees.
	Copyright (C) 2009-2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_TRIANGLETREES

#ifndef OCTREE_H_
#define OCTREE_H_

#include "TriangleAccessor.h"
#include "TriangleTree.h"
#include <cstdint>
#include <memory>
#include <vector>

namespace Geometry {
template<typename value_t> class _Box;
typedef _Box<float> Box;
}
namespace Rendering {
class Mesh;
}
namespace Util {
class AttributeProvider;
}
namespace MinSG {
namespace TriangleTrees {

/**
 * One object of this class represents a node in an octree.
 *
 * @author Benjamin Eikel
 * @date 2011-07-26
 */
class Octree : public TriangleTree {
	public:
		/**
		 * Create an Octree root node with TriangleAccessors to all given triangles.
		 *
		 * @param mesh Mesh containing the triangles.
		 * @param trianglesPerNode Maximum number of triangles to store inside a node. Bigger nodes will be split.
		 * @param enlargementFactor Increase of the size of the bounding box. Must not be smaller than one. If larger than one, this Octree will be an Loose Octree.
		 */
		MINSGAPI explicit Octree(Rendering::Mesh * mesh, uint32_t trianglesPerNode, float enlargementFactor);

		//! Clean up memory for possible children.
		MINSGAPI virtual ~Octree();

		/**
		 * Tell if the node is a leaf node.
		 *
		 * @return @c true if the node is a leaf and @c false if it is an inner node.
		 */
		bool isLeaf() const override {
			return children.empty();
		}

		/**
		 * Return the loose factor of this node.
		 *
		 * @return Loose factor
		 */
		float getLooseFactor() const {
			return looseFactor;
		}

		/**
		 * Retrieve the eight child nodes.
		 *
		 * @return Pointers to children.
		 */
		std::vector<const TriangleTree *> getChildren() const override {
			std::vector<const TriangleTree *> childArray;
			childArray.reserve(children.size());
			for(const auto & child : children) {
				childArray.push_back(child.get());
			}
			return childArray;
		}

		/**
		 * Return one triangle.
		 *
		 * @param index Index of triangle stored in this node.
		 * @return Indices stored in this node.
		 */
		MINSGAPI const TriangleAccessor & getTriangle(uint32_t index) const override;

		/**
		 * Return the number of triangles that are stored in this tree node.
		 *
		 * @return Triangle count
		 */
		uint32_t getTriangleCount() const override {
			return triangleStorage.size();
		}

		/**
		 * Check if this node should be split.
		 *
		 * @return @c true if this node should be split.
		 */
		bool shouldSplit() const {
			return getTriangleCount() > maxTrianglesPerNode;
		}

		/**
		 * Split this node if it is a leaf. It creates two children
		 * and converts this node into an inner node.
		 */
		MINSGAPI void split();

		/**
		 * Check if the triangle fits into the bounding box of the tree
		 * node.
		 *
		 * @param triangle Triangle to check.
		 * @return @c true if the triangle is inside or at most
		 * touching the bounds.
		 */
		MINSGAPI bool contains(const TriangleAccessor & triangle) const override;

		/**
		 * Add attributes specific to this object to the given container.
		 *
		 * @param container Container for attributes.
		 */
		MINSGAPI void fetchAttributes(Util::AttributeProvider * container) const override;

	protected:
		//! Internal storage of the triangles inside this node.
		std::vector<TriangleAccessor> triangleStorage;

		//! Eight children of this node or empty if leaf.
		std::vector<std::unique_ptr<Octree>> children;

		/**
		 * Maximum number of triangles stored in a node. If the actual
		 * number is bigger, the node will be split.
		 */
		const uint32_t maxTrianglesPerNode;

		//! Fraction of bounding box enlargement.
		const float looseFactor;

		/**
		 * Create a new Octree node. This is used to create child
		 * nodes. The creating node has to assign the triangles to the
		 * node.
		 *
		 * @param childBound Axis-aligned bounding box for the child.
		 * @param parent Parent node which is used to copy the
		 * parameters from.
		 */
		MINSGAPI explicit Octree(const Geometry::Box & childBound, const Octree & parent);

		//! Return a child node. Needed for polymorphism.
		virtual Octree * createChild(const Geometry::Box & childBound, const Octree & parent) const {
			return new Octree(childBound, parent);
		}
};

}
}

#endif /* OCTREE_H_ */

#endif /* MINSG_EXT_TRIANGLETREES */
