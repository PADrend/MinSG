/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "TransparencyRenderer.h"
#include "../Nodes/AbstractCameraNode.h"
#include "../Nodes/GroupNode.h"
#include "../FrameContext.h"
#include <Rendering/RenderingContext/RenderingParameters.h>
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Util/Macros.h>
#include <functional>

namespace MinSG {

TransparencyRenderer::TransparencyRenderer() : NodeRendererState(FrameContext::TRANSPARENCY_CHANNEL), nodes(), usePremultipliedAlpha(true) {
}

TransparencyRenderer::TransparencyRenderer(const TransparencyRenderer & source) :
	NodeRendererState(source), nodes(), usePremultipliedAlpha(source.usePremultipliedAlpha) {
}

TransparencyRenderer::~TransparencyRenderer() = default;

void TransparencyRenderer::addNode(Node * n) {
	if(!nodes) {
		WARN("State not enabled");
	} else {
		nodes->insert(n);
	}
}

State::stateResult_t TransparencyRenderer::doEnableState(FrameContext & context, Node * node, const RenderParam & rp) {
	nodes.reset(new DistanceSetB2F<Node>(context.getCamera()->getWorldOrigin()));
	return NodeRendererState::doEnableState(context, node, rp);
}

void TransparencyRenderer::doDisableState(FrameContext & context, Node * node, const RenderParam & rp) {
	NodeRendererState::doDisableState(context, node, rp);

	// No need to disable depth writing here because we are rendering in the correct order from back to front.
	Rendering::BlendingParameters blend;
	blend.enable();
	blend.setBlendColor( Util::Color4f(1.0,1.0,1.0,0.5) );

	if(usePremultipliedAlpha)
		blend.setBlendFunc(Rendering::BlendingParameters::ONE, Rendering::BlendingParameters::ONE_MINUS_SRC_ALPHA);
	else
		blend.setBlendFunc(Rendering::BlendingParameters::SRC_ALPHA, Rendering::BlendingParameters::ONE_MINUS_SRC_ALPHA);
	context.getRenderingContext().pushAndSetBlending(blend);

	RenderParam childParams(rp);
	childParams.setFlag(USE_WORLD_MATRIX);
	childParams.setChannel(FrameContext::TRANSPARENCY_CHANNEL);

	std::unique_ptr<DistanceSetB2F<Node>> tempNodes;
	tempNodes.swap(nodes);

	for (auto & elem : *tempNodes) {
		if(elem == node) {
			WARN("TransparencyRenderer is attached to a node that it shall display.\n" \
			     "         This may be caused by a BlendingState attached to the same node.\n" \
			     "         TransparencyRenderer has been disabled.\n        ");
			deactivate();
			break;
		} else {
			elem->display(context, childParams);
		}
	}
	context.getRenderingContext().popBlending();
}

NodeRendererResult TransparencyRenderer::displayNode(FrameContext &, Node * node, const RenderParam &) {
	nodes->insert(node);
	return NodeRendererResult::NODE_HANDLED;
}

TransparencyRenderer * TransparencyRenderer::clone() const {
	return new TransparencyRenderer(*this);
}

}
