/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "GroupState.h"
#include "../FrameContext.h"
#include <stdexcept>

namespace MinSG {

//! (static)
State * const GroupState::NO_STATE = nullptr;

GroupState::~GroupState() = default;

//!	---|> State
State::stateResult_t GroupState::doEnableState(FrameContext & context, Node * node, const RenderParam & rp) {
	enabledStates.push(NO_STATE); // add a marker (needed for nested activations)

	for(auto & state : states) {
		const State::stateResult_t result = state->enableState(context,node,rp);

		if(result==State::STATE_OK) {
			enabledStates.push(state.get());
		} else if(result==State::STATE_SKIP_OTHER_STATES) {
			enabledStates.push(state.get());
			return State::STATE_SKIP_OTHER_STATES;
		} else if(result==State::STATE_SKIP_RENDERING) {
			// if the rendering is skipped, its disableState function is not called, so we have to take care of this
			// to disable all prior enabled states.
			this->disableState(context,node,rp);
			return State::STATE_SKIP_RENDERING;
		} // else result==State::STATE_SKIPPED -> do nothing
	}
	return State::STATE_OK;
}

//!	---|> State
void GroupState::doDisableState(FrameContext & context, Node * node, const RenderParam & rp) {
	while(true){
		if(enabledStates.empty())
			throw std::logic_error("GroupState::disableState(...): Stack of enabledStates got empty.");
		State * const state = enabledStates.top();
		enabledStates.pop();
		if(state == NO_STATE) // marker found
			break;
		state->disableState(context,node,rp);
	}
}

void GroupState::addState(State * s) {
	if(isEnabled())
		throw std::logic_error("GroupState::addState(...) called on an enabled state.");
	states.push_back(s);
}

void GroupState::removeState(State * s) {
	if(isEnabled())
		throw std::logic_error("GroupState::removeState(...) called on an enabled state.");
	for(auto it=states.begin();it!=states.end();) {
		if( *it==s ) {
			it = states.erase(it);
			break;
		} else {
			++it;
		}
	}
}

void GroupState::removeStates() {
	if(isEnabled())
		throw std::logic_error("GroupState::removeStates(...) called on an enabled state.");
	states.clear();
}

}
