/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "AbstractBehaviour.h"
#include <Util/ObjectExtension.h>

namespace MinSG {

Util::Reference<BehaviorStatus> Behavior::createBehaviorStatus(){
	Util::Reference<BehaviorStatus> status = new BehaviorStatus(this);
	doPrepareBehaviorStatus(*status.get());
	return std::move(status);
}


Behavior::behaviourResult_t Behavior::execute(BehaviorStatus & state,double currentTimeSec){
	if(state.isNew()){
		state._init( currentTimeSec );
		doBeforeInitialExecute(state);
	}
	
	if(!state.isActive())
		return FINISHED;
	
	state._updateTime(currentTimeSec);
	const auto result = doExecute2(state);
	if(result == FINISHED)
		finalize(state); // automatic cleanup.

	return result;
}
void Behavior::finalize(BehaviorStatus & state){
	if(state.isActive()){
		state._markAsFinished();
		doFinalize(state);
	}
}

}
