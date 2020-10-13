/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef ABSTRACTTREEBUILDER_H_
#define ABSTRACTTREEBUILDER_H_

#include <Geometry/Box.h>

#include <Util/References.h>
#include <Util/GenericAttribute.h>

namespace MinSG {

class GroupNode;

namespace TreeBuilder {

class AbstractTreeBuilder {

public:

	struct NodeWrapper {

		Util::Reference<GroupNode> group;
		Geometry::Box tightBox;
		Geometry::Box looseBox;
		uint32_t depth;

		NodeWrapper() :
			group(nullptr), tightBox(), looseBox(), depth(0) {
			tightBox.invalidate();
			looseBox.invalidate();
		}

		NodeWrapper(Util::Reference<GroupNode> _group, Geometry::Box _tightBox, Geometry::Box _looseBox, uint32_t _depth) :
			group(std::move(_group)), tightBox(std::move(_tightBox)), looseBox(std::move(_looseBox)), depth(_depth) {
		}
	};

	MINSGAPI AbstractTreeBuilder(Util::GenericAttributeMap & options);
	MINSGAPI virtual ~AbstractTreeBuilder();

	MINSGAPI void buildTree(Util::Reference<GroupNode> group, const Geometry::Box & target);
	MINSGAPI void buildTree(Util::Reference<GroupNode> group);

protected:

	typedef std::deque<NodeWrapper> list_t;

	/**
	 * main method to build trees, splits the source into parts an then does recursive calls with each part
	 * - if canSplit --> split --> distribute --> finalize --> recurse
	 */
	MINSGAPI void buildTree(NodeWrapper & source);

	/**
	 * method to split the source
	 * should not set up the relationships between source an new created group nodes
	 */
	virtual list_t split(NodeWrapper & source) = 0;

	/**
	 * distributes the children of source into one of dest by
	 * first selecting the one out of dest where thight box contains the center of the child
	 * then moving the child into selected dest iff it fits into the loose box
	 */
	MINSGAPI static void distribute(NodeWrapper & source, list_t & dest);

	/**
	 * determines if a box can split thats true if
	 * - maximum tree depth is not reached
	 * - maximum child count in source is exceeded
	 */
	MINSGAPI bool canSplit(const NodeWrapper & source);

	/**
	 * removes empty entries from dest
	 * sets up the relationships between source and dest
	 */
	MINSGAPI void finalize(NodeWrapper & source, list_t & dest);

	uint32_t maxTreeDepth;
	uint32_t maxChildCount;
	float looseFactor;
	bool useGeometryBBs;
	bool prefereCubes;
	NodeWrapper root;

};

}
}

#endif /* ABSTRACTTREEBUILDER_H_ */
