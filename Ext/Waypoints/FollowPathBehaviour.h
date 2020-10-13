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

#ifndef __FollowPathBehaviour_H
#define __FollowPathBehaviour_H

#include "../../Core/Behaviours/AbstractBehaviour.h"
#include <vector>

namespace MinSG {
class PathNode;

/**
 * PathNode::FollowPathBehaviour
 * @ingroup behaviour
 */
class FollowPathBehaviour : public AbstractNodeBehaviour {
	PROVIDES_TYPE_NAME(FollowPathBehaviour)

	public:
		MINSGAPI FollowPathBehaviour(PathNode * pathNode, Node * node,float speed=1.0f);
		MINSGAPI virtual ~FollowPathBehaviour();

		void setPath(PathNode* pathNode)	{	path = pathNode;	}
		PathNode* getPath() const 			{	return path.get();	}
		float getSpeed()const 				{	return speed;	}
		void setSpeed(float newSpeed) 		{	speed=newSpeed;	}
		void setPosition(float newPosition,AbstractBehaviour::timestamp_t /*now*/) {
			position=newPosition;
		}
		float getPosition()const			{return position;	}

		MINSGAPI behaviourResult_t doExecute() override;

	private:
		Util::Reference<PathNode> path;
		float position;
		float speed;
};


}
#endif //__FollowPathBehaviour_H

#endif /* MINSG_EXT_WAYPOINTS */
