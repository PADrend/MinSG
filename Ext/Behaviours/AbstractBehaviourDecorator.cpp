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
#include "AbstractBehaviourDecorator.h"

namespace MinSG {

AbstractBehaviourDecorator::AbstractBehaviourDecorator(AbstractBehaviour* decorated):
		AbstractNodeBehaviour(nullptr),decoratedBehaviour(decorated) {
	AbstractNodeBehaviour * behaviour = dynamic_cast<AbstractNodeBehaviour *>(decoratedBehaviour.get());
	if(behaviour)
		setNode(behaviour->getNode());
}

AbstractBehaviour* AbstractBehaviourDecorator::getDecoratedRoot() {
	AbstractBehaviourDecorator* decorator;
	AbstractBehaviourDecorator* decorated = this;
	do {
		decorator = decorated;
		decorated = dynamic_cast<AbstractBehaviourDecorator*>(decorator->getDecorated());
	} while(decorated && decorated != this);
	return decorator->getDecorated();
}


void AbstractBehaviourDecorator::onNodeChanged(Node * /*oldNode*/) {
	AbstractNodeBehaviour * behaviour = dynamic_cast<AbstractNodeBehaviour *>(getDecorated());
	if(behaviour)
		behaviour->setNode(getNode());
}

}
