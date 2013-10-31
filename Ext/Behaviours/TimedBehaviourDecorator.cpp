/*
	This file is part of the MinSG library extension Behaviours.
	Copyright (C) 2011 Sascha Brandt
	Copyright (C) 2009-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2009-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2009-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "TimedBehaviourDecorator.h"

namespace MinSG {

TimedBehaviourDecorator::TimedBehaviourDecorator(AbstractBehaviour* decorated, timestamp_t _starttime, bool _relative)
	: AbstractBehaviourDecorator(decorated), startTime(_starttime), firstTime(0), relative(_relative) {}

TimedBehaviourDecorator::~TimedBehaviourDecorator() {}


AbstractBehaviour::behaviourResult_t TimedBehaviourDecorator::doExecute() {
	const timestamp_t timeSec = getCurrentTime();
	if(firstTime==0) {
		firstTime=timeSec;
	}
	timestamp_t time = relative ? timeSec-firstTime : timeSec;

	if(time < startTime) {
		return AbstractBehaviour::CONTINUE;
	} else {
		return getDecorated()->execute(time-startTime);
	}
}

}
