/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "OccludeeRenderer.h"
#include "OccRenderer.h"
#include "../../Core/States/PolygonModeState.h"
#include "../../Core/FrameContext.h"
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Util/Macros.h>

namespace MinSG {

OccludeeRenderer::OccludeeRenderer() : useWireframe(true), showOriginal(true), State(), occlusionCullingRenderer(new OccRenderer) {
}

OccludeeRenderer::OccludeeRenderer(const OccludeeRenderer & other) : useWireframe(other.useWireframe), showOriginal(other.showOriginal), State(other), occlusionCullingRenderer(other.occlusionCullingRenderer->clone()) {
}

OccludeeRenderer::~OccludeeRenderer() = default;

OccludeeRenderer * OccludeeRenderer::clone() const {
	return new OccludeeRenderer(*this);
}

State::stateResult_t OccludeeRenderer::doEnableState(FrameContext & context, Node * rootNode, const RenderParam & rp) {
	if (rp.getFlag(SKIP_RENDERER)) {
		return State::STATE_SKIPPED;
	}
	context.getRenderingContext().setImmediateMode(true);

	static PolygonModeState polygonMode;
	polygonMode.changeParameters().setMode(Rendering::PolygonModeParameters::LINE);
	
	if(!showOriginal)
		context.getRenderingContext().pushAndSetColorBuffer(Rendering::ColorBufferParameters(false,false,false,false));
		
	occlusionCullingRenderer->setMode(OccRenderer::MODE_CULLING);
	const State::stateResult_t resultCulling = occlusionCullingRenderer->enableState(context, rootNode, rp);
	if(resultCulling != State::STATE_SKIP_RENDERING) {
		WARN("Using the OccRenderer for culling failed.");
		return resultCulling;
	}
	if(!showOriginal)
		context.getRenderingContext().popColorBuffer();

	// Clear the depth buffer to be able to render the occluded geometry in front of the previously rendered geometry.
	context.getRenderingContext().clearDepth(1.0f);

	// Activate wireframe mode.
	if(useWireframe)
		polygonMode.enableState(context, rootNode, rp);

	occlusionCullingRenderer->setMode(OccRenderer::MODE_SHOW_CULLED);
	RenderParam rpWorldMatrix = rp;
	rpWorldMatrix.setFlag(USE_WORLD_MATRIX);
	const State::stateResult_t resultShowCulled = occlusionCullingRenderer->enableState(context, rootNode, rpWorldMatrix);
	if(resultShowCulled != State::STATE_SKIP_RENDERING) {
		WARN("Using the OccRenderer for showing the culled geometry failed.");
		return resultShowCulled;
	}

	// Deactivate wireframe mode.
	if(useWireframe)
		polygonMode.disableState(context, rootNode, rp);
	context.getRenderingContext().setImmediateMode(false);

	return State::STATE_SKIP_RENDERING;
}

}
