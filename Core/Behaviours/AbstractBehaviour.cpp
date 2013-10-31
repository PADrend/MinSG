/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "AbstractBehaviour.h"
#include "BehaviorStatusExtensions.h"
#include "../Nodes/Node.h"
#include "../States/State.h"

#include  <Util/ObjectExtension.h>

namespace MinSG {

// -------------------------------------------------------------------------------------------
// AbstractBehaviour

AbstractBehaviour::AbstractBehaviour() : Behavior(), myStatus(createBehaviorStatus()){
}


// -------------------------------------------------------------------------------------------
// AbstractNodeBehaviour
AbstractNodeBehaviour::AbstractNodeBehaviour(Node * node) : AbstractBehaviour(){

	//! \see BehaviorNodeReference
	Util::addObjectExtension<BehaviorNodeReference>(myStatus.get());
	Util::requireObjectExtension<BehaviorNodeReference>(myStatus.get())->setNode( node );
}


Node * AbstractNodeBehaviour::getNode()const	{	
	return Util::requireObjectExtension<BehaviorNodeReference>(myStatus.get())->getNode(); 	//! \see BehaviorNodeReference
}

/**
 * Sets the node of the AbstractNodeBehaviour.
 * @param node
 *
 * @note You should really know what you are doing when using this method,
 * because it can result in unpredictable behaviour.
 *
 * @note This method calls onNodeChanged(oldNode) to allow cleanup when needed.
 */
void AbstractNodeBehaviour::setNode(Node * newNode) {
	if(newNode!=getNode()){
		Util::Reference<Node> old = getNode();
		Util::requireObjectExtension<BehaviorNodeReference>(myStatus.get())->setNode(newNode); 	//! \see BehaviorNodeReference
//		nodeRef = newNode;
		onNodeChanged(old.get());
	}
}



// -------------------------------------------------------------------------------------------
// AbstractStateBehaviour

AbstractStateBehaviour::AbstractStateBehaviour(State * state) : AbstractBehaviour(){
	//! \see BehaviorStateReference
	Util::addObjectExtension<BehaviorStateReference>(myStatus.get());
	Util::requireObjectExtension<BehaviorStateReference>(myStatus.get())->setState( state );
}
State * AbstractStateBehaviour::getState()const	{	
	return Util::requireObjectExtension<BehaviorStateReference>(myStatus.get())->getState(); 	//! \see BehaviorStateReference
}


}
