/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "AbstractTreeBuilder.h"
#include "TreeBuilder.h"

#include "../../Helper/StdNodeVisitors.h"
#include "../../Helper/Helper.h"

#include "../../Core/Nodes/ListNode.h"

#include <Util/Macros.h>

#include "TreeBuilder.h"

namespace MinSG {
namespace TreeBuilder {

using MinSG::Node;
using MinSG::ListNode;
using MinSG::GroupNode;
using Util::Reference;
using Geometry::Box;
using std::deque;

AbstractTreeBuilder::AbstractTreeBuilder(Util::GenericAttributeMap & options) {
	maxTreeDepth = options.getUInt(MAX_TREE_DEPTH, 10);
	maxChildCount = options.getUInt(MAX_CHILD_COUNT, 8);
	looseFactor = options.getFloat(LOOSE_FACTOR, 2);
	useGeometryBBs = options.getBool(USE_GEOMETRY_BB, true);
	prefereCubes = options.getBool(PREFERE_CUBES, true);
}

AbstractTreeBuilder::~AbstractTreeBuilder() {

}

void AbstractTreeBuilder::buildTree(Reference<GroupNode> group, const Box & target) {
	buildList(group.get());
	root = NodeWrapper(group, target, target, 0);
	buildTree(root);
}

void AbstractTreeBuilder::buildTree(Reference<GroupNode> rootNode) {
	buildTree(rootNode, rootNode->getWorldBB());
}

void AbstractTreeBuilder::buildTree(NodeWrapper & source) {

	if (!canSplit(source))
		return;

	list_t dest = split(source);

	distribute(source, dest);

	finalize(source, dest);

	for (auto & elem : dest) {
		buildTree(elem);
	}
}

/* static */
void AbstractTreeBuilder::distribute(NodeWrapper & source, list_t & dest) {
	const auto children = getChildNodes(source.group.get());
	for (const auto & child : children) {
		for (auto & elem : dest) {
			if (elem.tightBox.contains(child->getWorldBB().getCenter()) && elem.looseBox.contains(child->getWorldBB())) {
				changeParentKeepTransformation(child, elem.group.get());
				//dest_iter->group->addChild(child);
				break;
			}
		}
	}
}

bool AbstractTreeBuilder::canSplit(const NodeWrapper & source) {
	if (source.depth >= maxTreeDepth)
		return false;
	if (source.group->countChildren() <= maxChildCount)
		return false;

	const auto children = getChildNodes(source.group.get());
	// children has at least size two
	Box b = (*children.begin())->getWorldBB();
	for(auto iter = children.begin()+1; iter != children.end(); ++iter) {
		if(b.getCenter() != (*iter)->getWorldBB().getCenter()) {
			return true;
		}
	}
	WARN("all remaining nodes have identical bounding boxes, stopping recursion");
	return false;
}

void AbstractTreeBuilder::finalize(NodeWrapper & source, list_t & dest) {
	list_t nonEmpty;
	for (auto & elem : dest) {
		if (elem.group->countChildren() != 0) {
			changeParentKeepTransformation(elem.group.get(), source.group.get());
			//source.group->addChild(iter->group.get());
			nonEmpty.push_back(elem);
		}
	}
	dest.swap(nonEmpty);
}

}
}
