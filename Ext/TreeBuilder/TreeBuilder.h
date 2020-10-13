/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef TREEBUILDER_H_
#define TREEBUILDER_H_

#include <Util/References.h>
#include <Util/GenericAttribute.h>

#include <string>
#include <cstdint>

namespace Geometry {
template<typename value_t> class _Box;
typedef _Box<float> Box;
}

namespace MinSG {

class ListNode;
class Node;
class GroupNode;

/**
 * Provides functions for reorganizing the data structure of scene graphs
 *
 * @note All of currently implemented functions expect an instance of ListNode to be passed as the scene root.
 *       Besides only the scene root and closed nodes are allowed to contain states.
 *
 * @author pajustus
 * @ingroup ext
 *
 */
namespace TreeBuilder {
	
//! Boolean. If set, bounding boxes will not always split in all dimensions (quadtree, octree, kd-tree).
//! If the ratio between maximum and minimum extent of the bounding box gets greater than squareroot of two, only the large dimensions are split.
const std::string PREFERE_CUBES = "PREFERE_CUBES";
//! Boolean. If set, bounding boxes of the geometry instead of those of the previous step are used for splitting (quadtree, octree, binary tree, kd-tree).
const std::string USE_GEOMETRY_BB = "USE_GEOMETRY_BB";
//! Number. The maximum depth of the created tree. Leaves in depth >= maximum will not be split.
const std::string MAX_TREE_DEPTH = "MAX_TREE_DEPTH";
//! Number. The maximum number of nodes stored in leaves. Leaves with more nodes will be split up as long as the maximum depth is not reached.
const std::string MAX_CHILD_COUNT = "MAX_CHILD_COUNT";
//! Number. The scale factor for boxes when inserting nodes. If you don't want a loose tree, set this value to one.
const std::string LOOSE_FACTOR = "LOOSE_FACTOR";
	//! Boolean. If set, the bounding box is expanded to a cube/square before splitting (quadtree, octree). Don't forget to disable `use geometry bounding boxes´.
const std::string EXACT_CUBES = "EXACT_CUBES";

//! Builds a binary tree by splitting allways the largest dimension.
MINSGAPI void buildBinaryTree(GroupNode * root, Util::GenericAttributeMap & options);
inline void buildBinaryTree(GroupNode * root) {
	Util::GenericAttributeMap map;
	buildBinaryTree(root, map);
}

//! Builds several variants of kd-trees.
MINSGAPI void buildKDTree(GroupNode * root, Util::GenericAttributeMap & options);
inline void buildKDTree(GroupNode * root) {
	Util::GenericAttributeMap map;
	buildKDTree(root, map);
}

//! Builds several variants of quadtrees.
MINSGAPI void buildQuadTree(GroupNode * root, Util::GenericAttributeMap & options);
inline void buildQuadTree(GroupNode * root) {
	Util::GenericAttributeMap map;
	buildQuadTree(root, map);
}

//! Builds several variants of octrees.
MINSGAPI void buildOcTree(GroupNode * root, Util::GenericAttributeMap & options);
inline void buildOcTree(GroupNode * root) {
	Util::GenericAttributeMap map;
	buildOcTree(root, map);
}

//! Builds a simple list.
//! @param options unused, just for compatibility with other build methods
MINSGAPI void buildList(GroupNode * root, Util::GenericAttributeMap & options);
inline void buildList(GroupNode * root) {
	Util::GenericAttributeMap map;
	buildList(root, map);
}

//! checks the preconditions to use a treebuilder, throws exception if not fulfilled
MINSGAPI void checkPreconditions(Util::Reference<GroupNode> group);

}

}

#endif // TREEBUILDER_H_
