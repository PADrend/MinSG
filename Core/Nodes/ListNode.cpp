/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "ListNode.h"
#include "../FrameContext.h"
#include "AbstractCameraNode.h"
#include <Geometry/BoxHelper.h>
#include <Rendering/Draw.h>
#include <Util/Graphics/ColorLibrary.h>
#include <Util/Macros.h>
#include <iterator>

namespace MinSG {

ListNode::ListNode() : GroupNode(), bbValid(false) {
}

ListNode::ListNode(const ListNode & source) : GroupNode(source), children(), bb(source.bb), bbValid(source.bbValid) {
	for(const auto & child : source.children) {
		if(!child.isNull()) // recursively clone children, WITHOUT attributes!
			addChild(child.get()->clone(false));
	}
}

ListNode::ListNode(ListNode && source) : GroupNode(source), children(source.children), bb(source.bb), bbValid(source.bbValid) {
	source.children.clear();
}

ListNode::~ListNode() {
	for(auto & child : children)
		child->_setParent(nullptr);
}

//! ---|> GroupNode
void ListNode::doAddChild(Util::Reference<Node> child) {
	_pushChild(child);
	child->_setParent(this);
}

//! ---|> GroupNode
bool ListNode::doRemoveChild(Util::Reference<Node> childToRemove) {
	for(auto child = children.begin(); child != children.end(); ++child) {
		if(*child == childToRemove) {
			children.erase(child);
			childToRemove->_setParent(nullptr);
			return true;
		}
	}
	return false;
}

//! ---|> [Node]
void ListNode::doDisplay(FrameContext & context, const RenderParam & rp) {
	if (rp.getFlag(BOUNDING_BOXES)) {
		// worldBB
		Rendering::drawAbsWireframeBox(context.getRenderingContext(), getWorldBB(), Util::ColorLibrary::LIGHT_GREY);
		// BB
		Rendering::drawWireframeBox(context.getRenderingContext(), getBB(), Util::ColorLibrary::WHITE);
	}

	if (rp.getFlag(FRUSTUM_CULLING)) {
		for(const auto & child : children) {
			int t = context.getCamera()->testBoxFrustumIntersection(child->getWorldBB());

			if (t == Geometry::Frustum::INSIDE) {
				context.displayNode(child.get(), rp - FRUSTUM_CULLING);
			} else if (t == Geometry::Frustum::INTERSECT) {
				context.displayNode(child.get(), rp);
			}
		}
	} else {
		for(const auto & child : children) {
			context.displayNode(child.get(), rp);
		}
	}
}

//! ---|> [Node]
const Geometry::Box& ListNode::doGetBB() const {
	if(!isBBValid()) {
		Geometry::Box newBB;
		newBB.invalidate();
		for(const auto & child : children) {
			if(child.isNull()) 
				continue;
			if(child->hasRelTransformation()) {
				newBB.include(Geometry::Helper::getTransformedBox(child->getBB(), child->getRelTransformationMatrix()));
			} else {
				newBB.include(child->getBB());
			}
		}
		bb = newBB;
		bbValid = true;
	}
	return bb;
}


//! ---|> [GroupNode]
size_t ListNode::countChildren() const {
	return children.size();
}

//! ---|> [Node]
NodeVisitor::status ListNode::traverse(NodeVisitor & visitor) {
	NodeVisitor::status status = visitor.enter(this);
	if (status == NodeVisitor::EXIT_TRAVERSAL) {
		return NodeVisitor::EXIT_TRAVERSAL;
	} else if (status == NodeVisitor::CONTINUE_TRAVERSAL) {
		for(auto & child : children) {
			if (child->traverse(visitor) == NodeVisitor::EXIT_TRAVERSAL) {
				return NodeVisitor::EXIT_TRAVERSAL;
			}
		}
	}
	return visitor.leave(this);
}

size_t ListNode::getMemoryUsage() const {
	size_t size = Node::getMemoryUsage() - sizeof(Node);
	size += sizeof(ListNode);
	size += children.size() * sizeof(childNodes_t::value_type);
	return size;
}

}
