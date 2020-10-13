/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef BEHAVIORBASE_H
#define BEHAVIORBASE_H

#include <Util/ReferenceCounter.h>
#include <Util/AttributeProvider.h>

namespace MinSG{
class Behavior;

class BehaviorStatus :
			public Util::ReferenceCounter<BehaviorStatus>,
			public Util::AttributeProvider {
	private:
		Util::Reference<Behavior> behavior;
		double startingTimeSec;
		double lastTimeSec;
		double currentTimeSec;
		enum state_t { 
			NEW,ACTIVE,FINISHED
		} state;

	public:
		MINSGAPI BehaviorStatus(Behavior * b);
		MINSGAPI ~BehaviorStatus();

		Behavior * getBehavior()const		{	return behavior.get();	}
		double getCurrentTime()const		{	return currentTimeSec;	}
		double getLastTime()const			{	return lastTimeSec;	}
		double getStartingTime()const		{	return startingTimeSec;	}
		double getTimeDelta()const			{	return currentTimeSec-lastTimeSec;	}
		bool isActive()const				{	return state == ACTIVE;	}
		bool isNew()const					{	return state == NEW;	}
		bool isFinished()const				{	return state == FINISHED;	}

	public:
		//! The following methods should not be called manually.
		void _init(double s)				{	startingTimeSec = lastTimeSec = currentTimeSec = s, state = ACTIVE;	}
		void _markAsFinished()				{	state = FINISHED;	}
		void _updateTime(double nowSec){
			lastTimeSec = currentTimeSec;
			currentTimeSec = nowSec;
		}

};


}
#endif // BEHAVIORBASE_H
