/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef SG_LISTNODE_H
#define SG_LISTNODE_H

#include "GroupNode.h"
#include <Geometry/Box.h>
#include <deque>

namespace MinSG {

/**
 *    [ListNode] ---|> [GroupNode] ---|> [Node]
 * @ingroup nodes
 */
class ListNode : public GroupNode {
		PROVIDES_TYPE_NAME(ListNode)
	public:
		MINSGAPI ListNode();
		MINSGAPI ListNode(ListNode && source);
		MINSGAPI virtual ~ListNode();

		Node * getChild(size_t index) const {
			return (index < children.size()) ? children[index].get() : nullptr;
		}

		/// ---|> [GroupNode]
		MINSGAPI size_t countChildren()const override;

		/// ---|> [Node]
		MINSGAPI NodeVisitor::status traverse(NodeVisitor & visitor)override;
		MINSGAPI void doDisplay(FrameContext & context,const RenderParam & rp)override;


		/**
		 * Get the amount of memory that is required to store this node. The
		 * returned value does not include the size of child nodes.
		 * 
		 * @return Amount of memory in bytes
		 */
		MINSGAPI size_t getMemoryUsage() const override;

	protected:
		MINSGAPI void doAddChild(Util::Reference<Node> child)override;
		MINSGAPI bool doRemoveChild(Util::Reference<Node> child)override;

		void _pushChild(Util::Reference<Node> child)	{	children.push_back(child);	}
		MINSGAPI explicit ListNode(const ListNode & source);
	private:
		/// ---|> [Node]
		MINSGAPI const Geometry::Box& doGetBB() const override;		
		ListNode * doClone() const override	{	return new ListNode(*this);	}

		/// ---|> [GroupNode]
		void invalidateCompoundBB()override {	bbValid=false;	}

		// Bounding Box
		bool isBBValid() const				{	return bbValid;	}

		typedef std::deque<Node::ref_t> childNodes_t;
		//! Direct children of this node.
		childNodes_t children;

		mutable Geometry::Box bb;
		mutable bool bbValid;
};

}

#endif // SG_LISTNODE_H
