/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "BehaviorStatusExtensions.h"
#include "../Nodes/Node.h"
#include <stdexcept>

namespace MinSG{

Node * BehaviorNodeReference::requireNode()const	{	
	if(node.isNull())
		throw std::logic_error("BehaviorNodeReference::requireNode: no Node set.");
	return node.get();
}

// -------------------------------

State * BehaviorStateReference::requireState()const	{	
	if(state.isNull())
		throw std::logic_error("BehaviorStateReference::requireState: no State set.");
	return state.get();
}

}
