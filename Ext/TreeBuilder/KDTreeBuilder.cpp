/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "KDTreeBuilder.h"
#include "TreeBuilder.h"

#include "../../Core/Nodes/ListNode.h"
#include "../../Helper/StdNodeVisitors.h"

#include <Geometry/Box.h>
#include <Geometry/Definitions.h>

#include <vector>
#include <algorithm>

namespace MinSG {

namespace TreeBuilder {

KDTreeBuilder::KDTreeBuilder(Util::GenericAttributeMap & options) :
	AbstractTreeBuilder(options) {
}

KDTreeBuilder::~KDTreeBuilder() {
}

struct AxisComparator {
	AxisComparator(const uint8_t _axis) :
		axis(_axis) {
	}

	bool operator()(Node * first, Node * second) const {
		return first->getWorldBB().getCenter().get(axis) < second->getWorldBB().getCenter().get(axis);
	}

	const uint8_t axis;
};

//! building kd-tree takes 2 seconds with the boing scene, so the focus in this method is on nice & understandable and not on efficiency
AbstractTreeBuilder::list_t KDTreeBuilder::split(NodeWrapper & source) {

	// select the box to be split, depending on options
	Geometry::Box toSplit;
	if (useGeometryBBs)
		toSplit = source.group->getWorldBB();
	else
		toSplit = source.tightBox;

	// select the axis to be used for splitting, depending on options
	uint8_t axis;
	if (prefereCubes) { // take the axis with largest extent
		if (	toSplit.getExtentX() > toSplit.getExtentY()
			 &&	toSplit.getExtentX() > toSplit.getExtentZ())
			axis = 0;
		else if (toSplit.getExtentY() > toSplit.getExtentZ())
			axis = 1;
		else
			axis = 2;
	}
	else{ // take alternating axis
		axis = source.depth % 3;
	}

	// search the node which is the median according to selected axis
	auto children = getChildNodes(source.group.get());

	std::nth_element(	children.begin(),
						children.begin() + children.size()/2,
						children.end(),
						AxisComparator(axis)	);

	// split the box
	Geometry::Box a = toSplit;
	Geometry::Box b = toSplit;
	float middle = children[children.size()/2]->getWorldBB().getCenter().get(axis); // coordinate of the median node
	a.setMax(static_cast<Geometry::dimension_t>(axis), middle);
	b.setMin(static_cast<Geometry::dimension_t>(axis), middle);

	// create child list for next level
	list_t dest;
	NodeWrapper nw;
	nw = NodeWrapper(new ListNode(), a, a, source.depth+1);
	nw.looseBox.resizeRel(looseFactor);
	dest.push_back(nw);
	nw = NodeWrapper(new ListNode(), b, b, source.depth+1);
	nw.looseBox.resizeRel(looseFactor);
	dest.push_back(nw);

	return dest;

}

}

}
