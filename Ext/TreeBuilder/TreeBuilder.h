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
 * This class provides functions for reorganizing the data structure of scene graphs
 *
 * @note All of currently implemented functions expect an instance of ListNode to be passed as the scene root.
 *       Besides only the scene root and closed nodes are allowed to contain states.
 *
 * @author pajustus
 * @ingroup ext
 *
 */
namespace TreeBuilder {

const std::string PREFERE_CUBES = "PREFERE_CUBES";
const std::string USE_GEOMETRY_BB = "USE_GEOMETRY_BB";
const std::string MAX_TREE_DEPTH = "MAX_TREE_DEPTH";
const std::string MAX_CHILD_COUNT = "MAX_CHILD_COUNT";
const std::string LOOSE_FACTOR = "LOOSE_FACTOR";
const std::string EXACT_CUBES = "EXACT_CUBES";

void buildBinaryTree(GroupNode * root, Util::GenericAttributeMap & options);
inline void buildBinaryTree(GroupNode * root) {
	Util::GenericAttributeMap map;
	buildBinaryTree(root, map);
}

void buildKDTree(GroupNode * root, Util::GenericAttributeMap & options);
inline void buildKDTree(GroupNode * root) {
	Util::GenericAttributeMap map;
	buildKDTree(root, map);
}

void buildQuadTree(GroupNode * root, Util::GenericAttributeMap & options);
inline void buildQuadTree(GroupNode * root) {
	Util::GenericAttributeMap map;
	buildQuadTree(root, map);
}

void buildOcTree(GroupNode * root, Util::GenericAttributeMap & options);
inline void buildOcTree(GroupNode * root) {
	Util::GenericAttributeMap map;
	buildOcTree(root, map);
}

//! @param options unused, just for compatibility with other build methods
void buildList(GroupNode * root, Util::GenericAttributeMap & options);
inline void buildList(GroupNode * root) {
	Util::GenericAttributeMap map;
	buildList(root, map);
}

//! checks the preconditions to use a treebuilder, throws exception if not fulfilled
void checkPreconditions(Util::Reference<GroupNode> group);

}

}

#endif // TREEBUILDER_H_
