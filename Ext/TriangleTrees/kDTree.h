/*
	This file is part of the MinSG library extension TriangleTrees.
	Copyright (C) 2009-2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_TRIANGLETREES

#ifndef KDTREE_H_
#define KDTREE_H_

#include "TriangleTree.h"
#include <cstddef>
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
class TriangleAccessor;

/**
 * One object of this class represents a node in a k-D-tree. To be exact
 * this class does not work with k dimensions but with exactly three
 * dimensions.
 *
 * @author Benjamin Eikel
 * @date 2009-06-30
 */
class kDTree : public TriangleTree {
	public:
		/**
		 * Create a new kDTree node with the given triangles. This
		 * creates a root node which creates a copy of the triangles.
		 * Furthermore it builds a axis-aligned bounding box around the
		 * triangles.
		 *
		 * @param mesh Mesh containing the triangles.
		 * @param trianglesPerNode Maximum number of triangles to store
		 * inside a node. Bigger nodes will be split.
		 * @param allowedBBEnlargement Maximum increase of the size of
		 * the bounding box when triangles are inserted. If a triangle
		 * fits into the at most increased bounding box it will not be
		 * cut. This parameter is the fraction of the original bounding
		 * box size that is used for increase. For example if the value
		 * is @c 0.1f then a maximum increase of 10% is allowed.
		 */
		MINSGAPI explicit kDTree(Rendering::Mesh * mesh,
				uint32_t trianglesPerNode = 1000,
				float allowedBBEnlargement = 0.0f);

		//! Clean up memory for possible children.
		MINSGAPI virtual ~kDTree();

		/**
		 * Tell if the node is a leaf node.
		 *
		 * @return @c true if the node is a leaf and @c false if it is
		 * an inner node.
		 */
		bool isLeaf() const override {
			return (firstChild.get() == nullptr);
		}

		/**
		 * Retrieve the two child nodes.
		 *
		 * @return Pointers to children.
		 */
		std::vector<const TriangleTree *> getChildren() const override {
			std::vector<const TriangleTree *> children;
			children.push_back(firstChild.get());
			children.push_back(secondChild.get());
			return children;
		}

		/**
		 * Return the dimension which is orthogonal to the splitting
		 * plane.
		 *
		 * @return Dimension X = 0, Y = 1, Z = 2
		 */
		virtual uint8_t getSplitDimension() const {
			return getLevel() % 3;
		}

		/**
		 * Return the coordinate value of the splitting
		 * plane.
		 *
		 * @return Coordinate in split dimension
		 */
		float getSplitValue() const {
			return splitValue;
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
			return sorted[getSplitDimension()].size();
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
		/**
		 * Internal storage of the triangles inside the nodes. The
		 * root node allocates this storage and hands the pointer
		 * to the children.
		 */
		std::vector<TriangleAccessor> * triangleStorage;

		/**
		 * These array of vectors contains the indices of the
		 * triangles inside @a triangleStorage. The three vectors are
		 * sorted by each of the three coordinates of the center of the
		 * triangles. The vectors are only used during the construction
		 * of the tree and freed afterwards.
		 */
		std::vector<uint32_t> sorted[3];

		/**
		 * Coordinate value of the splitting plane in the splitting
		 * dimension.
		 */
		float splitValue;

		//! First child of this node or @c nullptr if leaf.
		std::unique_ptr<kDTree> firstChild;

		//! Second child of this node or @c nullptr if leaf.
		std::unique_ptr<kDTree> secondChild;

		/**
		 * Maximum number of triangles stored in a node. If the actual
		 * number is bigger, the node will be split.
		 */
		const uint32_t maxTrianglesPerNode;

		//! Fraction of allowed bounding box increase.
		const float maxBoundingBoxEnlargement;

		/**
		 * Create a new kDTree node. This is used to create child
		 * nodes. The creating node has to assign the triangles to the
		 * node.
		 *
		 * @param childBound Axis-aligned bounding box for the child.
		 * @param parent Parent node which is used to copy the
		 * parameters from.
		 */
		MINSGAPI explicit kDTree(const Geometry::Box & childBound, const kDTree & parent);

		//! Return a child node. Needed for polymorphism.
		virtual kDTree * createChild(const Geometry::Box & childBound, const kDTree & parent) const {
			return new kDTree(childBound, parent);
		}

		/**
		 * Calculate the value of the splitting plane in the current
		 * splitting dimension.
		 *
		 * @param [out] numFirstChild Number of triangles that will be
		 * assigned to the first child. This value has to be reliable.
		 * @param [out] numSecondChild Number of triangles that will be
		 * assigned to the second child. This value has to be reliable.
		 * assigned to the first child. This value has to be reliable.
		 * @note It has the side effect that the @a splitValue will be
		 * set to the calculated value.
		 */
		MINSGAPI virtual void calculateSplittingPlane(uint32_t & numFirstChild, uint32_t & numSecondChild);

		/**
		 * Check if the triangle given by its @a index lies on both
		 * sides of the splitting plane given by @a splitValue in the
		 * @a splitDimension.
		 *
		 * @param triangle Triangle to check
		 * @param splitDimension Dimension orthogonal to the splitting
		 * plane
		 * @param splitValue Coordinate value of the splitting plane
		 * @param [out] minPos Minimum coordinate of the vertices of
		 * the triangle
		 * @param [out] maxPos Maximum coordinate of the vertices of
		 * the triangle
		 * @param @c true if the triangle lies on both side, @c false
		 * otherwise
		 */
		MINSGAPI static bool needCut(const TriangleAccessor & triangle,
							uint8_t splitDimension,
							float splitValue, float & minPos,
							float & maxPos);
};

}
}

#endif /* KDTREE_H_ */

#endif /* MINSG_EXT_TRIANGLETREES */
