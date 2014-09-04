/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_WAYPOINTS

#include "FollowPathBehaviour.h"

#include "PathNode.h"

#include "../../Core/Behaviours/BehaviourManager.h"

#include <Util/Macros.h>

#include <iostream>

namespace MinSG {

/**
 * [ctor] FollowPathBehaviour
 */
FollowPathBehaviour::FollowPathBehaviour(PathNode * _path, Node * _node,float _speed/*=1.0f*/)
		: AbstractNodeBehaviour(_node), path(_path),position(0),speed(_speed){
}
/**
 * [dtor] FollowPathBehaviour
 */
FollowPathBehaviour::~FollowPathBehaviour() {
}

/**
 * FollowPathBehaviour ---|> AbstractBehaviour
 */
AbstractBehaviour::behaviourResult_t FollowPathBehaviour::doExecute() {
	if (getNode()==nullptr || path.isNull()) {
		return CONTINUE;
	}
	position = position + getTimeDelta() * speed;

	// don't update the position while we're not moving
	if(speed == 0.0f) {
		return CONTINUE;
	}
	Geometry::SRT srt=path->getWorldPosition( position );

	// convert the absolute path srt into coordinate system of the parent.
	if(getNode()->hasParent()){
		srt=getNode()->getParent()->getWorldToLocalMatrix() * srt ;
	}

	getNode()->setRelTransformation(srt);
	if(position >= path->getMaxTime() && !path->isLooping())
		return FINISHED;
	return CONTINUE;
}

}

#endif /* MINSG_EXT_WAYPOINTS */
