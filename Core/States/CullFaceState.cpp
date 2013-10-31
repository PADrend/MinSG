/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "CullFaceState.h"
#include "../FrameContext.h"
#include <Rendering/RenderingContext/RenderingContext.h>

namespace MinSG {

State::stateResult_t CullFaceState::doEnableState(FrameContext & context, Node * /*node*/, const RenderParam & /*rp*/) {
	context.getRenderingContext().pushAndSetCullFace(getParameters());
	return State::STATE_OK;
}

void CullFaceState::doDisableState(FrameContext & context, Node *, const RenderParam & /*rp*/) {
	context.getRenderingContext().popCullFace();
}

}
