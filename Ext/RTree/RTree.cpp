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

#include "RTree.h"
#include "../../Core/FrameContext.h"
#include "../../Helper/Helper.h"
#include <Util/Graphics/ColorLibrary.h>
#include <Util/Macros.h>
#include <set>

namespace MinSG {

RTree::RTree(uint16_t minEntries, uint16_t maxEntries) :
	ListNode(), isRoot(true), isLeaf(true), m(minEntries), M(maxEntries) {
	if (minEntries > (maxEntries / 2)) {
		WARN("m has to be smaller or equal to M/2.");
	}
}

RTree::RTree(const RTree & source) :
	ListNode(), isRoot(false), isLeaf(source.isLeaf), m(source.m), M(source.M) {
	// Do not call the copy constructor of ListNode here!
}

void RTree::doAddChild(Util::Reference<Node> child) {
	// [Find position for new record]
	RTree * leaf = chooseLeaf(child->getWorldBB());
	RTree * newLeaf = nullptr;
	// [Add record to leaf node]
	if (leaf->countChildren() < leaf->M) {
		leaf->_pushChild(child);
		child->_setParent(leaf);
	} else {
		newLeaf = splitNode(leaf, child.get());
	}
	// [Propagate changes upward]
	adjustTree(leaf, newLeaf);
	// [Grow tree taller]
	if (leaf->isRoot && newLeaf != nullptr) {
		// Instead of creating a new root, re-use the current root.
		// This keeps all pointers to the tree valid.
		auto replacement = new RTree(*leaf);

		const size_t childCount = leaf->countChildren();
		for (size_t i = 0; i < childCount; ++i) {
			Node * childOfLeaf = leaf->getChild(i);
			replacement->_pushChild(childOfLeaf);
			child->_setParent(replacement);
		}

		// Assign new children to the root node.
		leaf->clearChildren();
		leaf->_pushChild(replacement);
		replacement->_setParent(leaf);
		leaf->_pushChild(newLeaf);
		newLeaf->_setParent(leaf);
		leaf->isLeaf = false;
	}
}

bool RTree::doRemoveChild(Util::Reference<Node> child) {
	if (dynamic_cast<RTree *> (child.get()) != nullptr) {
		WARN("An internal node cannot be removed directly. Remove the entries in the leaf nodes instead.");
		return false;
	}
	// [Find node containing record]
	RTree * leaf = findLeaf(this, child.get());
	if (leaf == nullptr) {
		return false;
	}
	// [Delete record]
	leaf->removeChild(child.get());
	// [Propagate changes]
	condenseTree(leaf);
	// [Shorten tree]
	if (isRoot && !isLeaf && countChildren() == 1) {
		// Pull up the children of the single node.
		Util::Reference<RTree> single = static_cast<RTree *> (getChild(0));
		clearChildren();
		const size_t childCount = single->countChildren();
		for (size_t i = 0; i < childCount; ++i) {
			Node * childOfSingle = single->getChild(i);
			childOfSingle->_setParent(this);
			_pushChild(childOfSingle);
		}
		isLeaf = single->isLeaf;
		single->clearChildren();
		single->_setParent(nullptr);
		MinSG::destroy(single.get());
	}
	return true;
}

std::string RTree::toString() const {
	std::ostringstream s;
	s << "(RTree:" << reinterpret_cast<const void *>(this);
	if (isRoot) {
		s << ",root";
	}
	if (isLeaf) {
		s << ",leaf";
	}
	s << ")";
	return s.str();
}

RTree * RTree::chooseLeaf(const Geometry::Box & newBox) {
	// [Initialize]
	RTree * n = this;
	// [Leaf check]
	while (!n->isLeaf) {
		// [Choose subtree]
		float minEnlargement = std::numeric_limits<float>::max();
		float minVolume = std::numeric_limits<float>::max();
		RTree * minN = nullptr;
		const size_t childCount = n->countChildren();
		for (size_t i = 0; i < childCount; ++i) {
			Node * child = n->getChild(i);
			Geometry::Box box = child->getWorldBB();
			float childVolume = box.getVolume();
			box.include(newBox);
			float enlargement = box.getVolume() - childVolume;
			if (enlargement < minEnlargement) {
				minEnlargement = enlargement;
				minVolume = childVolume;
				minN = dynamic_cast<RTree *> (child);
			} else if (enlargement == minEnlargement && childVolume < minVolume) {
				minVolume = childVolume;
				minN = dynamic_cast<RTree *> (child);
			}
		}
		n = minN;
	}
	// [Descend until a leaf is reached]
	return n;
}

void RTree::adjustTree(RTree * & n, RTree * & nn) {
	// [Initialize]
	// Done. See parameters.
	// [Check if done]
	while (!n->isRoot) {
		// [Adjust covering rectangle in parent entry]
		// Automatically done by MinSG.
		// [Propagate node split upward]
		if (nn == nullptr) {
			return;
		}
		RTree * p = dynamic_cast<RTree *> (n->getParent());
		RTree * pp = nullptr;
		if (p->countChildren() < p->M) {
			p->_pushChild(nn);
			nn->_setParent(p);
		} else {
			pp = splitNode(p, nn);
		}
		// [Move up to next level]
		n = p;
		nn = pp;
	}
}

RTree * RTree::findLeaf(RTree * root, Node * node) {
	const size_t childCount = root->countChildren();
	// [Search subtrees]
	if (!root->isLeaf) {
		const Geometry::Box & nodeBox = node->getWorldBB();
		for (size_t i = 0; i < childCount; ++i) {
			Node * child = root->getChild(i);
			const Geometry::Box & box = child->getWorldBB();
			if (box.contains(nodeBox)) {
				RTree * result = findLeaf(static_cast<RTree *> (child), node);
				if (result != nullptr) {
					return result;
				}
			}
		}
		return nullptr;
	}
	// [Search leaf node for record]
	for (size_t i = 0; i < childCount; ++i) {
		if (root->getChild(i) == node) {
			return root;
		}
	}
	return nullptr;
}

void RTree::condenseTree(RTree * leaf) {
	/*
	 * ##############################
	 * Important notice:
	 * In contrast to the algorithm description in the underlying article, orphaned inner nodes of the tree are not re-inserted.
	 * Instead, the contents of their subtrees are collected and re-inserted with the standard insertion method.
	 * ##############################
	 */
	// [Initialize]
	RTree * n = leaf;
	// Tuples of nodes and their height (counting from leaves to root).
	std::deque<Node::ref_t> q;
	while (!n->isRoot) {
		// [Find parent entry]
		RTree * p = dynamic_cast<RTree *> (n->getParent());
		// [Eliminate under-full node]
		if (n->countChildren() < n->m) {
			// Store children in set.
			collectEntries(n, q);
			// Delete node from parent.
			p->removeChild(n);
			MinSG::destroy(n);
		}
		// [Adjust covering rectangle]
		// Automatically done by MinSG.
		// [Move up one level in tree]
		n = p;
	}
	// [Re-insert orphaned entries]
	for (auto & elem : q) {
		elem.get()->_setParent(nullptr);
		// n is the root node now. See condition of while loop.
		n->addChild(elem.get());
	}
}

void RTree::collectEntries(RTree * node, std::deque<Node::ref_t> & entries) {
	const size_t childCount = node->countChildren();
	for (size_t i = 0; i < childCount; ++i) {
		Node * child = node->getChild(i);
		if (node->isLeaf) {
			entries.push_back(child);
		} else {
			collectEntries(static_cast<RTree *> (child), entries);
		}
	}
	node->clearChildren();
	node->_setParent(nullptr);
	MinSG::destroy(node);
}

RTree * RTree::splitNode(RTree * node, Node * element) {
	// Accumulate the child nodes.
	std::list<Node::ref_t> nodes;
	const size_t childCount = node->countChildren();
	for (size_t i = 0; i < childCount; ++i) {
		nodes.push_back(node->getChild(i));
	}
	nodes.push_back(element);

	node->clearChildren();


	// Distribute the accumulated nodes to two lists.
	std::deque<Node::ref_t> first;
	std::deque<Node::ref_t> second;
	distributeNodes(nodes, first, second, node->m);

	for (auto & elem : first) {
		node->_pushChild(elem.get());
		elem.get()->_setParent(node);
	}

	auto newLeaf = new RTree(*node);
	for (auto & elem : second) {
		newLeaf->_pushChild(elem.get());
		elem.get()->_setParent(newLeaf);
	}

	return newLeaf;
}

void RTree::distributeNodes(std::list<Node::ref_t> & input, std::deque<Node::ref_t> & first, std::deque<Node::ref_t> & second, uint32_t m) {
	// [Pick first entry for each group]
	{
		//! @see Algorithm PickSeeds
		float maxD = std::numeric_limits<float>::lowest();
		std::list<Node::ref_t>::iterator maxIt1;
		std::list<Node::ref_t>::iterator maxIt2;
		for (auto it1 = input.begin(); it1 != input.end(); ++it1) {
			for (auto it2 = input.begin(); it2 != input.end(); ++it2) {
				// [Calculate inefficiency of grouping entries together]
				const Geometry::Box & box1 = (*it1)->getWorldBB();
				const Geometry::Box & box2 = (*it2)->getWorldBB();
				Geometry::Box j(box1);
				j.include(box2);
				const float d = j.getVolume() - box1.getVolume() - box2.getVolume();
				if (d > maxD) {
					maxD = d;
					maxIt1 = it1;
					maxIt2 = it2;
				}
			}
		}
		first.push_back(*maxIt1);
		second.push_back(*maxIt2);
		input.erase(maxIt1);
		input.erase(maxIt2);
	}
	Geometry::Box firstBox(first.front()->getWorldBB());
	Geometry::Box secondBox(second.front()->getWorldBB());
	// [Check if done]
	while (!input.empty()) {
		size_t numLeft = input.size();
		if (first.size() + numLeft == m) {
			first.insert(first.end(), input.begin(), input.end());
			return;
		}
		if (second.size() + numLeft == m) {
			second.insert(second.end(), input.begin(), input.end());
			return;
		}
		// [Select entry to assign]
		//! @see Algorithm PickNext
		float firstVolume = firstBox.getVolume();
		float secondVolume = secondBox.getVolume();
		float maxDiff = std::numeric_limits<float>::lowest();
		std::list<Node::ref_t>::iterator maxIt;
		bool addToFirst = false;
		// [Determine cost of putting each entry in each group]
		for (auto it = input.begin(); it != input.end(); ++it) {
			Geometry::Box box1((*it)->getWorldBB());
			box1.include(firstBox);
			Geometry::Box box2((*it)->getWorldBB());
			box2.include(secondBox);
			const float enlargement1 = box1.getVolume() - firstVolume;
			const float enlargement2 = box2.getVolume() - secondVolume;
			// [Find entry with greatest preference for one group]
			const float diff = std::abs(enlargement1 - enlargement2);
			if (diff > maxDiff) {
				maxDiff = diff;
				maxIt = it;
				if (enlargement1 == enlargement2) {
					if (firstVolume == secondVolume) {
						addToFirst = first.size() < second.size();
					} else {
						addToFirst = firstVolume < secondVolume;
					}
				} else {
					addToFirst = enlargement1 < enlargement2;
				}
			}
		}
		if (addToFirst) {
			first.push_back(*maxIt);
			firstBox.include((*maxIt)->getWorldBB());
		} else {
			second.push_back(*maxIt);
			secondBox.include((*maxIt)->getWorldBB());
		}
		input.erase(maxIt);
	}
}

}

#endif /* MINSG_EXT_RTREE */
