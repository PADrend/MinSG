/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "QuadTreeBuilder.h"
#include "TreeBuilder.h"

#include "../../Core/Nodes/ListNode.h"

#include <Geometry/Box.h>
#include <Geometry/BoxHelper.h>

namespace MinSG {

namespace TreeBuilder {

QuadTreeBuilder::QuadTreeBuilder(Util::GenericAttributeMap & options) :
	OcTreeBuilder(options) {
}

QuadTreeBuilder::~QuadTreeBuilder() {
}

AbstractTreeBuilder::list_t QuadTreeBuilder::split(NodeWrapper & source) {

	if (source.depth == 0 && exactCubes) {
		float maxi = std::max(source.tightBox.getExtentX(), source.tightBox.getExtentZ());
		source.tightBox.setMaxX(source.tightBox.getMinX() + maxi);
		source.tightBox.setMaxZ(source.tightBox.getMinZ() + maxi);
		source.looseBox = source.tightBox;
		source.looseBox.resizeRel(looseFactor);
	}

	static float sq2 = sqrt(2);
	uint32_t x = 2, y = 1, z = 2;
	Geometry::Box toSplit;

	if (useGeometryBBs)
		toSplit = source.group->getWorldBB();
	else
		toSplit = source.tightBox;

	float ex = toSplit.getExtentX();
	float ez = toSplit.getExtentZ();

	if (prefereCubes && (ex / ez > sq2 || ez / ex > sq2)) {
		if (ex / ez <= sq2)
			x = 1;
		if (ez / ex <= sq2)
			z = 1;
	}

	const auto boxes = Geometry::Helper::splitUpBox(toSplit, x, y, z);

	list_t dest;
	for(const auto & tight : boxes) {
		auto ln = new ListNode();
		Geometry::Box loose(tight);
		loose.resizeRel(looseFactor);
		dest.emplace_back(ln, tight, loose, source.depth + 1);
	}
	return dest;

}

}

}
