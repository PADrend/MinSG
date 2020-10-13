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
#ifndef TIMEDBEHAVIOURDECORATOR_H_
#define TIMEDBEHAVIOURDECORATOR_H_

#include "../../Core/Behaviours/AbstractBehaviour.h"
#include "AbstractBehaviourDecorator.h"

namespace MinSG {
	
//! @ingroup behaviour
class TimedBehaviourDecorator: public AbstractBehaviourDecorator {
	PROVIDES_TYPE_NAME(TimedBehaviourDecorator)
public:
	MINSGAPI explicit TimedBehaviourDecorator(AbstractBehaviour* decorated, timestamp_t starttime=0, bool relative=false);
	MINSGAPI virtual ~TimedBehaviourDecorator();

	void setStartTime(timestamp_t time, bool relativeStarttime=false ) {
		startTime = time;
		relative=relativeStarttime;
	}
	timestamp_t getStartTime() { return startTime; }

	MINSGAPI behaviourResult_t doExecute() override;
private:
	timestamp_t startTime;
	timestamp_t firstTime;
	bool relative;
};

}

#endif /* TIMEDBEHAVIOURDECORATOR_H_ */
