/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "BehaviorStatus.h"
#include "../Nodes/Node.h"
#include <Util/ObjectExtension.h>
#include "AbstractBehaviour.h"

/*


Open questions:
 - speed control
 - stopped -> finished
 - explicit void Behavior::start(...) ? or implicitly on first execute ?
 - Is it possible to convert AbstractBehavior into Behavior?
 - How to query a node's behaviors?
 - How to query a state's behaviors?




decorator
group
timeController?????




Node --> FollowPathBehavior






*/



namespace MinSG{
	
BehaviorStatus::BehaviorStatus(Behavior * b) :
	behavior(b),
	startingTimeSec(0.0),lastTimeSec(0.0),currentTimeSec(0.0),
	state(NEW) {}

BehaviorStatus::~BehaviorStatus(){}

}
