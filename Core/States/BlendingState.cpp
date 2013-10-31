/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "BlendingState.h"
#include "../FrameContext.h"
#include <Rendering/RenderingContext/ParameterStructs.h>
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Util/Graphics/Color.h>

namespace MinSG {

BlendingState::BlendingState() : RenderingParametersState<Rendering::BlendingParameters>(), depthWritesEnabled(true) {
	// init default values
	changeParameters().enable();
	changeParameters().setBlendFunc(Rendering::BlendingParameters::CONSTANT_ALPHA, Rendering::BlendingParameters::ONE_MINUS_CONSTANT_ALPHA);
	changeParameters().setBlendColor(Util::Color4f(0.0f, 0.0f, 0.0f, 0.5f));
}

State::stateResult_t BlendingState::doEnableState(FrameContext & context, Node * node, const RenderParam & rp) {
	// if the current rendering channel is not the TRANSPARENCY_CHANNEL, try to pass the node to that channel.
	if(rp.getChannel() != FrameContext::TRANSPARENCY_CHANNEL) {
		RenderParam newRp(rp);
		newRp.setChannel(FrameContext::TRANSPARENCY_CHANNEL);
		if(context.displayNode(node, newRp)) {
			// successfully passed on to a renderer at that channel
			return State::STATE_SKIP_RENDERING;
		}
	}
	// current render channel is TRANSPARENCY_CHANNEL or the node could not be passed on to that channel.
	context.getRenderingContext().pushAndSetBlending(getParameters());
	context.getRenderingContext().pushAndSetDepthBuffer(
		Rendering::DepthBufferParameters(
			context.getRenderingContext().getDepthBufferParameters().isTestEnabled(),
			getBlendDepthMask(),
			context.getRenderingContext().getDepthBufferParameters().getFunction()
		)
	);
	return State::STATE_OK;
}

void BlendingState::doDisableState(FrameContext & context, Node *, const RenderParam & /*rp*/) {
	context.getRenderingContext().popDepthBuffer();
	context.getRenderingContext().popBlending();
}

}
