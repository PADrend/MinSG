/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef SG_GROUPNODE_H
#define SG_GROUPNODE_H

#include "Node.h"

namespace MinSG {

/**
 *    (abstract)[GroupNode] ---|> [Node]
 * @ingroup nodes
 */
class GroupNode : public Node {
	PROVIDES_TYPE_NAME(GroupNode)
	public:
		typedef Util::Reference<GroupNode> ref_t;
		// -----------------

		MINSGAPI GroupNode();
		virtual ~GroupNode(){}

		/**
		 * ---o
		 *
		 * @return The number of direct children of this node.
		 */
		virtual size_t countChildren() const = 0;

		/*!	Add a child to this node and update the child's parent.
			- May throw an exception on failure	*/
		MINSGAPI void addChild(Util::Reference<Node> child);

		bool hasChildren()const		{	return countChildren()>0;	}

		/*!	Try to remove a child from this node.
			@return @c true if @a child was removed.	*/
		MINSGAPI bool removeChild(Util::Reference<Node> child);
		
		/*! (internal) Remove the given child from this node.
			- called by removeChild(...).
			- Has to set the child's parent to null (child->_setParent(nullptr)).
			@return false iff the node could not be found.
			\note Normally, use removeChild(...) instead.
			\todo make private!
			---o	*/
		virtual bool doRemoveChild(Util::Reference<Node> child) = 0;
		

		/*!	This method is called by Node::worldBBChanged() for all parent nodes (until one has a fixed bb).
			If the node's bounding box is influenced by its children, it should be invalidated here and 
			then re-calculated on the next call of doGetBB().
		 */
		virtual void invalidateCompoundBB() = 0;
		
		//! ---|> Node
		void removeFixedBB() override final{
			Node::removeFixedBB();
			invalidateCompoundBB(); // make sure that the compound bb is recalculated.
		}

		//! Removes all children from the Node
		MINSGAPI void clearChildren();

	private:
		/*! (internal) Add the given child to this node.
			- called by addChild(...).                            *
			- May throw an exception on failure (of base type std::exception).
			- Has to set the child's parent (child->_setParent(...)).
			- The given @p child can be assumed to be not null, which has been removed from its old parent.
			---o	*/
		virtual void doAddChild(Util::Reference<Node> child)=0;
};

}

#endif // SG_GROUPNODE_H
