/*
	This file is part of the MinSG library extension Physics.
	Copyright (C) 2013 Mouns Almarrani
	Copyright (C) 2013 Claudius Jähn <claudius@uni-paderborn.de>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_PHYSICS

#include "MotionState.h"
#include <Geometry/SRT.h>
#include "../../../Core/Nodes/Node.h"
#include "../../../Core/Transformations.h"
#include "BtPhysicWorld.h"

namespace MinSG {
namespace Physics {

void MotionState::setWorldTransform(const btTransform &worldTrans) {
    Node* node = physObj.getNode();
    if(!node){
		return; // silently return before we set a node( should not happen!)
	}else if(node->isDestroyed()){
		world.removeNode(node);
	}else{
		const float nodeScale = node->getScale();
		Geometry::SRT relSRT = Transformations::worldSRTToRelSRT(node, toSRT(worldTrans));
		relSRT.setScale( nodeScale );
		relSRT.translate( -Transformations::localDirToRelDir(node,physObj.getCenterOfMass()));
		node->setSRT(relSRT);
	}
}

}
}











#endif // MINSG_EXT_PHYSICS
