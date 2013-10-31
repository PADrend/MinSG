/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "TreeBuilder.h"

#include "BinaryTreeBuilder.h"
#include "OcTreeBuilder.h"
#include "QuadTreeBuilder.h"
#include "KDTreeBuilder.h"

#include <Util/References.h>
#include <Util/Macros.h>

#include <Geometry/Box.h>

#include "../../Core/Nodes/ListNode.h"
#include "../../Helper/StdNodeVisitors.h"
#include "../../Helper/Helper.h"

using namespace Geometry;
using namespace Util;

namespace MinSG {

namespace TreeBuilder {

void buildBinaryTree(GroupNode * root, Util::GenericAttributeMap& options) {
	BinaryTreeBuilder tb(options);
	tb.buildTree(root);
	std::cout << "TreeBuilder: built binary-tree" << std::endl;
}

void buildKDTree(GroupNode * root, Util::GenericAttributeMap & options) {
	KDTreeBuilder tb(options);
	tb.buildTree(root);
	std::cout << "TreeBuilder: built kd-tree" << std::endl;
}

void buildQuadTree(GroupNode * root, Util::GenericAttributeMap & options) {
	QuadTreeBuilder tb(options);
	tb.buildTree(root);
	std::cout << "TreeBuilder: built quad-tree" << std::endl;
}

void buildOcTree(GroupNode * root, Util::GenericAttributeMap & options) {
	OcTreeBuilder tb(options);
	tb.buildTree(root);
	std::cout << "TreeBuilder: built oct-ree" << std::endl;
}

static void checkPreCondtions(Util::Reference<GroupNode> group) {

	class Visitor: public NodeVisitor {

		Node * root;

	public:

		Visitor() :
			root(nullptr) {
		}
		~Visitor() {
		}

		status enter(Node * node) override {
			if (node->isClosed())
				return BREAK_TRAVERSAL;
			if (node != root && node->hasStates()) {
				throw std::logic_error("scene contains states in inner nodes");
			}
			return CONTINUE_TRAVERSAL;
		}

		void visit(GroupNode * node) {
			root = node;
			node->traverse(*this);
		}

	} visitor;

	visitor.visit(group.get());
		
}

void buildList(GroupNode * group, Util::GenericAttributeMap & /*options*/) {

	checkPreCondtions(group);

	const auto closedNodes = collectClosedNodes(group);
	std::deque<Reference<Node> > closed;
	for(const auto & closedNode : closedNodes) {
		closed.emplace_back(closedNode);
		changeParentKeepTransformation(closedNode, nullptr);
	}

	const auto children = getChildNodes(group);
	for(auto & child : children) {
		destroy(child);
	}
	
	for(const auto & child : closed) {
		changeParentKeepTransformation(child.get(), group);
	}

	std::cout << "TreeBuilder: built list" << std::endl;
}

}
}
