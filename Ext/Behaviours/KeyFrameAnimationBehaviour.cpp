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
#include "KeyFrameAnimationBehaviour.h"
#include "../KeyFrameAnimation/KeyFrameAnimationNode.h"

namespace MinSG {

KeyFrameAnimationBehaviour::KeyFrameAnimationBehaviour(KeyFrameAnimationNode * node) :
	AbstractNodeBehaviour(node) {
	node->setBehaviour(this);
}

KeyFrameAnimationBehaviour::~KeyFrameAnimationBehaviour() {
	//dtor
}

AbstractBehaviour::behaviourResult_t KeyFrameAnimationBehaviour::doExecute() {
	const timestamp_t timeSec = getCurrentTime();
	if ((static_cast<KeyFrameAnimationNode *>(getNode()))->updateMesh(timeSec)) {
		return AbstractBehaviour::CONTINUE;
	}
	return AbstractBehaviour::FINISHED;
}

}
