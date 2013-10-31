/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "MaterialState.h"
#include "../FrameContext.h"
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Util/Graphics/Color.h>

namespace MinSG {

State::stateResult_t MaterialState::doEnableState(FrameContext & context, Node * node, const RenderParam & rp) {
	// If the ambient color's alpha value or the diffuse color's alpha value is less than 1.0f, consider the material transparent.
	// The specular color's alpha value represents the reflectiveness of the material and is ignored.
	const bool isTransparent = getParameters().getAmbient().getA() < 1.0f || getParameters().getDiffuse().getA() < 1.0f;
	if(isTransparent) {
		// if the current rendering channel is not the TRANSPARENCY_CHANNEL, try to pass the node to that channel.
		if(rp.getChannel() != FrameContext::TRANSPARENCY_CHANNEL) {
			RenderParam newRp(rp);
			newRp.setChannel(FrameContext::TRANSPARENCY_CHANNEL);
			if(context.displayNode(node, newRp)) {
				// successfully passed on to a renderer at that channel
				return State::STATE_SKIP_RENDERING;
			}
		}
	}

	context.getRenderingContext().pushAndSetMaterial(getParameters());
	return State::STATE_OK;
}

void MaterialState::doDisableState(FrameContext & context, Node * /*node*/, const RenderParam & /*rp*/) {
	context.getRenderingContext().popMaterial();
}

void MaterialState::preMultiplyAlpha() {
	{
		Util::Color4f ambient = getParameters().getAmbient();
		ambient.setR(ambient.getR() * ambient.getA());
		ambient.setG(ambient.getG() * ambient.getA());
		ambient.setB(ambient.getB() * ambient.getA());
		changeParameters().setAmbient(ambient);
	}
	{
		Util::Color4f diffuse = getParameters().getDiffuse();
		diffuse.setR(diffuse.getR() * diffuse.getA());
		diffuse.setG(diffuse.getG() * diffuse.getA());
		diffuse.setB(diffuse.getB() * diffuse.getA());
		changeParameters().setDiffuse(diffuse);
	}
	{
		Util::Color4f specular = getParameters().getSpecular();
		specular.setR(specular.getR() * specular.getA());
		specular.setG(specular.getG() * specular.getA());
		specular.setB(specular.getB() * specular.getA());
		changeParameters().setSpecular(specular);
	}
}

}
