/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "LightingState.h"
#include "../Nodes/LightNode.h"
#include "../FrameContext.h"
#include <Util/References.h>

namespace MinSG {

LightingState::LightingState() :
	State(), enableLight(true), light(nullptr) {
}

LightingState::LightingState(LightNode * newLight) :
	State(), enableLight(true), light(newLight) {
}

LightingState::LightingState(const LightingState & source) :
	State(source), enableLight(source.enableLight),  light(source.light) {
}

State::stateResult_t LightingState::doEnableState(FrameContext & context, Node * /*node*/, const RenderParam & /*rp*/) {
    if( !light || !light->isActive()) 
		return State::STATE_SKIPPED;

	originalState = light->isSwitchedOn();
	if(enableLight){
		if(!originalState)
			light->switchOn(context);
	}else{
		if(originalState)
			light->switchOff(context);
	}

	return State::STATE_OK;
}

void LightingState::doDisableState(FrameContext & context, Node * /*node*/, const RenderParam & /*rp*/) {
	if(light){
		if(originalState){
			if(!light->isSwitchedOn())
				light->switchOn(context);
		}else{
			if(light->isSwitchedOn())
				light->switchOff(context);
		}
	}
}

LightingState * LightingState::clone() const {
	return new LightingState(*this);
}

}
