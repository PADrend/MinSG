/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "NodeRendererState.h"
#include "State.h"
#include "../FrameContext.h"
#include <functional>

namespace MinSG {

NodeRendererState::NodeRendererState(Util::StringIdentifier newChannel) :
	State(), nodeRendererChannel(newChannel) {
}


State::stateResult_t NodeRendererState::doEnableState(FrameContext & context, Node * /*node*/, const RenderParam & rp) {
	if(rp.getFlag(SKIP_RENDERER)) {
		return State::STATE_SKIPPED;
	}
	using namespace std::placeholders;
	context.pushNodeRenderer(nodeRendererChannel, std::bind(&NodeRendererState::displayNode, this, _1, _2, _3));
	return State::STATE_OK;
}

void NodeRendererState::doDisableState(FrameContext & context, Node * /*node*/, const RenderParam & rp) {
	if(rp.getFlag(SKIP_RENDERER)) {
		return;
	}
	context.popNodeRenderer(nodeRendererChannel);
}

}
