/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "BinaryTreeBuilder.h"
#include "TreeBuilder.h"

#include "../../Core/Nodes/ListNode.h"

#include <Geometry/Box.h>
#include <Geometry/BoxHelper.h>

namespace MinSG {

namespace TreeBuilder {

using Geometry::Box;

BinaryTreeBuilder::BinaryTreeBuilder(Util::GenericAttributeMap & options) : AbstractTreeBuilder(options){

}

BinaryTreeBuilder::~BinaryTreeBuilder() {

}

AbstractTreeBuilder::list_t BinaryTreeBuilder::split(NodeWrapper & source) {

	Box toSplit;
	if(useGeometryBBs)
		toSplit = source.group->getWorldBB();
	else
		toSplit = source.tightBox;
	std::vector<Box> boxes;
	if (toSplit.getExtentX() > toSplit.getExtentY() && toSplit.getExtentX() > toSplit.getExtentZ())
		boxes = Geometry::Helper::splitUpBox(toSplit, 2, 1, 1);
	else if (toSplit.getExtentZ() > toSplit.getExtentY())
		boxes = Geometry::Helper::splitUpBox(toSplit, 1, 1, 2);
	else
		boxes = Geometry::Helper::splitUpBox(toSplit, 1, 2, 1);

	list_t dest;
	for(const auto & tight : boxes) {
		auto ln = new ListNode();
		Box loose = Box(tight);
		loose.resizeRel(looseFactor);
		dest.emplace_back(ln, tight, loose, source.depth + 1);
	}

	return dest;
}

}

}
