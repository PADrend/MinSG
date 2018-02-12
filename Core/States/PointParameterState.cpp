/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	Copyright (C) 2018 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "PointParameterState.h"
#include "../FrameContext.h"
#include <Rendering/RenderingContext/RenderingContext.h>

namespace MinSG {

State::stateResult_t PointParameterState::doEnableState(FrameContext & context, Node * /*node*/, const RenderParam & /*rp*/) {
	context.getRenderingContext().pushAndSetPointParameters(getParameters());
	return State::STATE_OK;
}

void PointParameterState::doDisableState(FrameContext & context, Node * /*node*/, const RenderParam & /*rp*/) {
	context.getRenderingContext().popPointParameters();
}

}
