/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "OcTreeBuilder.h"
#include "TreeBuilder.h"

#include "../../Core/Nodes/ListNode.h"

#include <Geometry/Box.h>
#include <Geometry/BoxHelper.h>

namespace MinSG {

namespace TreeBuilder {

using Geometry::Box;

OcTreeBuilder::OcTreeBuilder(Util::GenericAttributeMap & options) :
	AbstractTreeBuilder(options) {
	exactCubes = options.getBool(EXACT_CUBES, false);
}

OcTreeBuilder::~OcTreeBuilder() {

}

AbstractTreeBuilder::list_t OcTreeBuilder::split(NodeWrapper & source) {

	if (source.depth == 0 && exactCubes) {
		float maxi = source.tightBox.getExtentMax();
		source.tightBox.setMaxX(source.tightBox.getMinX() + maxi);
		source.tightBox.setMaxY(source.tightBox.getMinY() + maxi);
		source.tightBox.setMaxZ(source.tightBox.getMinZ() + maxi);
		source.looseBox = source.tightBox;
		source.looseBox.resizeRel(looseFactor);
	}

	Box toSplit;

	if (useGeometryBBs)
		toSplit = source.group->getWorldBB();
	else
		toSplit = source.tightBox;

	std::vector<Box> newBoxes;
	if (prefereCubes)
		newBoxes = Geometry::Helper::splitBoxCubeLike(toSplit);
	else
		newBoxes = Geometry::Helper::splitUpBox(toSplit, 2, 2, 2);

	list_t dest;
	for(const auto & tight : newBoxes) {
		auto ln = new ListNode();
		Box loose(tight);
		loose.resizeRel(looseFactor);
		dest.emplace_back(ln, tight, loose, source.depth + 1);
	}

	return dest;

}

}

}
