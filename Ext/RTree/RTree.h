/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_RTREE

#ifndef RTREE_H_
#define RTREE_H_

#include "../../Core/Nodes/ListNode.h"
#include <deque>
#include <list>

namespace MinSG {

/**
 * Node which can be an internal or a leaf node of a R-tree.
 *
 * @author Benjamin Eikel
 * @date 2010-05-03
 * @see Antonin Guttman. R-trees: a dynamic index structure for spatial searching.
 * In SIGMOD ’84: Proceedings of the 1984 ACM SIGMOD international conference on Management of data, pages 47–57, New York, NY, USA, 1984.
 * @see http://doi.acm.org/10.1145/602259.602266
 * @ingroup nodes
 */
class RTree : public ListNode {
	PROVIDES_TYPE_NAME(RTree)
	public:
		/**
		 * Create a new RTree node. The node is set to be a leaf node and the root node of a tree.
		 *
		 * @param minEntries Minimum number of nodes in a node of the tree. This has to be at most half of the maximum number.
		 * @param maxEntries Maximum number of nodes in a node of the tree.
		 */
		MINSGAPI RTree(uint16_t minEntries, uint16_t maxEntries);

		/**
		 * Insert a new node into the tree.
		 *
		 * @param child New node to add to the tree.
		 * @see Algorithm Insert
		 */
		MINSGAPI void doAddChild(Util::Reference<Node> child) override;

		/**
		 * Remove a node from the tree.
		 *
		 * @param child Node which should be searched and removed.
		 * @return @c true if child was removed and @c false if the child was not found in the tree.
		 * @see Algorithm Delete
		 */
		MINSGAPI bool doRemoveChild(Util::Reference<Node> child) override;

		//! Return a string representation describing this node.
		MINSGAPI std::string toString() const;

		/**
		 * Find all nodes which intersect with the given box.
		 *
		 * @param query Query box with is used for intersection test.
		 * @param nodes List of nodes that are found.
		 * @see Algorithm Search
		 */
		// TODO: Implement.
		//void search(const Geometry::Box & query, std::deque<Node *> & nodes);

	private:
		//! Flag which tells if this is the root node of the tree.
		bool isRoot;
		//! Flag which tells if this is a leaf node.
		bool isLeaf;
		//! Minimum number of entries in a node.
		const uint16_t m;
		//! Maximum number of entries in a node.
		const uint16_t M;

		/**
		 * Private copy constructor. Set only the member variables for this class and do not call parent copy constructors.
		 *
		 * @param source Source tree node
		 */
		MINSGAPI RTree(const RTree & source);

		/**
		 * Select a leaf node, in which to place a new node.
		 *
		 * @param newBox Bounding box of the node, which is to be inserted.
		 * @return Leaf node
		 * @see Algorithm ChooseLeaf
		 */
		MINSGAPI virtual RTree * chooseLeaf(const Geometry::Box & newBox);

		/**
		 * Ascend from a leaf node to the root and propagate splits upwards.
		 *
		 * @param n Leaf node.
		 * @param nn Leaf node which is a sibling of @a leaf and is the result of a split.
		 * @see Algorithm AdjustTree
		 */
		MINSGAPI static void adjustTree(RTree * & n, RTree * & nn);

		/**
		 * Find the leaf node which contains the given node.
		 *
		 * @param root Root node of the subtree to search.
		 * @param node Node, which is searched for.
		 * @return Leaf node
		 * @see Algorithm FindLeaf
		 */
		MINSGAPI static RTree * findLeaf(RTree * root, Node * node);

		/**
		 * Check if the leaf node has enough nodes after deleting a node from it. Propagate changes upwards in the tree.
		 *
		 * @param leaf Leaf node, from which a node was deleted.
		 * @see Algorithm CondenseTree
		 */
		MINSGAPI static void condenseTree(RTree * leaf);

		/**
		 * Traverse the tree given by its root node and collect all entries that are stored in leaf nodes.
		 * The entries are removed from the nodes.
		 *
		 * @param node Root node of the subtree.
		 * @param entries Container to store the entries, that are found.
		 */
		MINSGAPI static void collectEntries(RTree * node, std::deque<Node::ref_t> & entries);

		/**
		 * Split an internal or a leaf node into two nodes and distribute their elements and the given additional element to them.
		 *
		 * @param node Leaf node or internal node.
		 * @param element Additional element to insert.
		 * @return New node.
		 * @see Algorithm Quadratic Split
		 */
		MINSGAPI static RTree * splitNode(RTree * node, Node * element);

		/**
		 * Distribute the given nodes to the two containers. The bounding boxes are the criteria for the distribution:
		 * The algorithms tries to generate small enclosing bounding volumes for the two containers.
		 *
		 * @param input Container of input nodes. The container will be empty when the function returns.
		 * @param first First group of output nodes.
		 * @param second Second group of output nodes.
		 * @param m Minimum number of entries in a node.
		 */
		MINSGAPI static void distributeNodes(std::list<Node::ref_t> & input, std::deque<Node::ref_t> & first, std::deque<Node::ref_t> & second, uint32_t m);
};

}

#endif /* RTREE_H_ */

#endif /* MINSG_EXT_RTREE */
