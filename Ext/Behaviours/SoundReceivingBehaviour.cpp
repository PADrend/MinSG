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
#ifdef MINSG_EXT_SOUND

#include "SoundReceivingBehaviour.h"
#include "../../Core/Nodes/Node.h"

namespace MinSG {

//! (ctor)
SoundReceivingBehaviour::SoundReceivingBehaviour(Node * node) :
		AbstractNodeBehaviour(node),
		lastPosition(node->getWorldOrigin()),lastTime(0) {
	//ctor
}

//!	---|> AbstractBehaviour
AbstractBehaviour::behaviourResult_t SoundReceivingBehaviour::doExecute() {
	const timestamp_t timeSec = getCurrentTime();
	if(lastTime==0)
		lastTime=timeSec;
	const timestamp_t tDiff = timeSec-lastTime;

	using namespace Geometry;
	const Vec3 newPos( getNode()->getWorldOrigin() );

	// update velocity
	Vec3 velocity( tDiff>0 ? (newPos-lastPosition)*(1.0/tDiff) : Vec3(0,0,0));
	float oldX,oldY,oldZ;
	getListener()->getVelocity(oldX,oldY,oldZ);
	velocity = (velocity+Vec3(oldX,oldY,oldZ)) * 0.5;
	getListener()->setVelocity(velocity.x(),velocity.y(),velocity.z());

	// update position
	getListener()->setPosition(newPos.x(),newPos.y(),newPos.z());

	// orientation
	const Vec3 dir( getNode()->getWorldTransformationMatrix().transformDirection(0,0,-1) );
	const Vec3 up( getNode()->getWorldTransformationMatrix().transformDirection(0,1,0) );
	getListener()->setOrientation(dir.x(),dir.y(),dir.z(),up.x(),up.y(),up.z());

	lastTime = timeSec;
	lastPosition = newPos;

	return CONTINUE;
}

}

#endif // MINSG_EXT_SOUND
