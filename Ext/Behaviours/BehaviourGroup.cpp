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
#include "BehaviourGroup.h"

namespace MinSG {

BehaviourGroup::BehaviourGroup() : AbstractNodeBehaviour(nullptr) {}

BehaviourGroup::~BehaviourGroup() {}

void BehaviourGroup::addBehaviour(AbstractBehaviour *behaviour) {
	if(!behaviour)
		return;
	Util::Reference<AbstractBehaviour> ref(behaviour);
	behaviours.push_back(ref);
//	behaviour->activate();
}

void BehaviourGroup::removeBehaviour(AbstractBehaviour *behaviour) {
	if(!behaviour)
		return;
	Util::Reference<AbstractBehaviour> ref(behaviour);
	behaviours.remove(ref);
//	behaviour->finalize();
}

void BehaviourGroup::getBehaviours(behaviourList_t & list) {
	list.insert(list.end(), behaviours.begin(), behaviours.end());
}

size_t BehaviourGroup::count()  {
	return behaviours.size();
}

AbstractBehaviour::behaviourResult_t BehaviourGroup::doExecute() {
	const timestamp_t timeSec = getCurrentTime();

	for(auto it = behaviours.begin(); it != behaviours.end();) {
		if( (*it)->execute(timeSec)==AbstractBehaviour::CONTINUE ){
			++it;
		}else{ // FINISHED
			behaviours.erase(it++);
		}
	}
	return behaviours.empty() ? AbstractBehaviour::FINISHED : AbstractBehaviour::CONTINUE;
}


void BehaviourGroup::onNodeChanged(Node * /*oldNode*/) {
	for(auto & behaviour : behaviours) {
		AbstractNodeBehaviour * nodeBehaviour = dynamic_cast<AbstractNodeBehaviour *>(behaviour.get());
		if(nodeBehaviour != nullptr) {
			nodeBehaviour->setNode(getNode());
		}
	}
}

}
