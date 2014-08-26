/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "ProjSizeFilterState.h"

#include "../../Core/Nodes/AbstractCameraNode.h"
#include "../../Core/FrameContext.h"
#include <Geometry/Box.h>
#include <functional>
#include <sstream>

using namespace Util;
using namespace Rendering;

namespace MinSG {

ProjSizeFilterState::ProjSizeFilterState() :
	NodeRendererState(FrameContext::DEFAULT_CHANNEL),
	maximumProjSize(1.0f),
	minimumDistance(0.0f),
	targetChannel(FrameContext::APPROXIMATION_CHANNEL),
	forceClosed(false) {
}

ProjSizeFilterState * ProjSizeFilterState::clone() const {
	return new ProjSizeFilterState(*this);
}

NodeRendererResult ProjSizeFilterState::displayNode(FrameContext & context, Node * node, const RenderParam & rp) {
	if (!forceClosed || !node->isClosed()) {
		if (minimumDistance > 0) {
			AbstractCameraNode * cam = context.getCamera();
			if (cam != nullptr && node->getWorldBB().getDistance(cam->getWorldOrigin()) < minimumDistance) // too close? -> do nothing
				return NodeRendererResult::PASS_ON;
		}
		Geometry::Rect projection = context.getProjectedRect(node);
		if (projection.getHeight() * projection.getWidth() > maximumProjSize) // too large? -> do nothing
			return NodeRendererResult::PASS_ON;
	}
	RenderParam newRP(rp);
	newRP.setChannel(targetChannel);
	context.displayNode(node, newRP);
	return NodeRendererResult::NODE_HANDLED;
}

}
