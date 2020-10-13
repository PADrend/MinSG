/*
	This file is part of the MinSG library extension TriangleTrees.
	Copyright (C) 2009-2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_TRIANGLETREES

#ifndef ABTREE_H_
#define ABTREE_H_

#include "kDTree.h"
#include <cstdint>

namespace Geometry {
template<typename value_t> class _Box;
typedef _Box<float> Box;
}
namespace Rendering {
class Mesh;
}
namespace MinSG {
namespace TriangleTrees {

/**
 * Adaptive binary tree as presented in Game Programming Gems 6,
 * Chapter 5.2: Spatial Partitioning Using an Adaptive Binary Tree.
 *
 * @author Benjamin Eikel
 * @date 2009-07-07
 */
class ABTree : public kDTree {
	public:
		/**
		 * Create a new kDTree node with the given triangles. This
		 * creates a root node which creates a copy of the triangles.
		 * Furthermore it builds a axis-aligned bounding box around the
		 * triangles.
		 *
		 * @param triangles List of triangles.
		 * @param trianglesPerNode Maximum number of triangles to store
		 * inside a node. Bigger nodes will be split.
		 * @param allowedBBEnlargement Maximum increase of the size of
		 * the bounding box when triangles are inserted. If a triangle
		 * fits into the at most increased bounding box it will not be
		 * cut. This parameter is the fraction of the original bounding
		 * box size that is used for increase. For example if the value
		 * is @c 0.1f then a maximum increase of 10% is allowed.
		 */
		MINSGAPI explicit ABTree(Rendering::Mesh * mesh,
				uint32_t trianglesPerNode = 1000,
				float allowedBBEnlargement = 0.0f);

		//! Does nothing.
		MINSGAPI virtual ~ABTree();

		/**
		 * Return the dimension which is orthogonal to the splitting
		 * plane.
		 *
		 * @return Dimension X = 0, Y = 1, Z = 2
		 */
		unsigned char getSplitDimension() const override {
			return splitDimension;
		}

	protected:
		/**
		 * In this tree the split dimension does not depend on the
		 * level of the node as it does in the kDTree. Therefore the
		 * split dimension has to be stored here.
		 */
		unsigned char splitDimension;

		/**
		 * Create a new kDTree node. This is used to create child
		 * nodes. The creating node has to assign the triangles to the
		 * node.
		 *
		 * @param childBound Axis-aligned bounding box for the child.
		 * @param parent Parent node which is used to copy the
		 * parameters from.
		 */
		MINSGAPI explicit ABTree(const Geometry::Box & childBound, const kDTree & parent);

		//! Return a child node. Needed for polymorphism.
		ABTree * createChild(const Geometry::Box & childBound, const kDTree & parent) const override {
			return new ABTree(childBound, parent);
		}


		/**
		 * Calculate the value of the splitting value and the splitting
		 * dimension by sampling multiple candidate planes.
		 *
		 * @param [out] numFirstChild Number of triangles that will be
		 * assigned to the first child. This value has to be reliable.
		 * @param [out] numSecondChild Number of triangles that will be
		 * assigned to the second child. This value has to be reliable.
		 * assigned to the first child. This value has to be reliable.
		 * @note It has the side effect that the @a splitValue will be
		 * set to the calculated value.
		 */
		MINSGAPI void calculateSplittingPlane(uint32_t & numFirstChild, uint32_t & numSecondChild) override;
};

}
}

#endif /* ABTREE_H_ */

#endif /* MINSG_EXT_TRIANGLETREES */
