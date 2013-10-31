/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "EnvironmentState.h"
#include "../../Core/Nodes/AbstractCameraNode.h"
#include "../../Core/Nodes/Node.h"
#include "../../Core/FrameContext.h"
#include <Rendering/RenderingContext/RenderingContext.h>

namespace MinSG {

//! [ctor]
EnvironmentState::EnvironmentState(Node * _environment/*=nullptr*/) :
	State(), environment(_environment) {
}

//! [dtor]
EnvironmentState::~EnvironmentState() {
	environment = nullptr;
	//dtor
}

//! ---|> [State]
State::stateResult_t EnvironmentState::doEnableState(FrameContext & context, Node * /*node*/, const RenderParam & rp) {

	if (rp.getFlag(NO_GEOMETRY))
		return State::STATE_SKIPPED;

	if (!environment.isNull()) {

		// extract rotation of the camera
		Geometry::SRT camRotationSrt = context.getRenderingContext().getCameraMatrix()._toSRT();
		camRotationSrt.setTranslation(Geometry::Vec3(0,0,0));
		camRotationSrt.setScale(1.0);

		// render the environment node as seen standing at the origin (0,0,0) but looking in the camera's direction.
		context.getRenderingContext().pushMatrix();
		context.getRenderingContext().setMatrix(Geometry::Matrix4x4(camRotationSrt));
		context.getRenderingContext().multMatrix(Geometry::Matrix4x4f(Geometry::SRT(Geometry::Vec3f(0,0,0), context.getWorldFrontVector(), context.getWorldUpVector())));
		context.displayNode(environment.get(), rp);
		context.getRenderingContext().popMatrix();

	}

	return State::STATE_OK;
}

EnvironmentState * EnvironmentState::clone() const {
	return new EnvironmentState(*this);
}

}
