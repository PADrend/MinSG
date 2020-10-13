/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef BEHAVIOR_STATUS_EXTENSIONS_H
#define BEHAVIOR_STATUS_EXTENSIONS_H
#include "../Nodes/Node.h"
#include "../States/State.h"
#include <Util/ReferenceCounter.h>
#include <Util/TypeNameMacro.h>

namespace MinSG{
class Node;
class State;

class BehaviorNodeReference{
		PROVIDES_TYPE_NAME_NV(BehaviorNodeReference)
		Util::Reference<Node> node;
	public:
		Node * getNode()const	{	return node.get();	}
		void setNode(Node * n)	{	node = n;	}
		//! Return the referenced Node or throw an exception if not set.
		MINSGAPI Node * requireNode()const;
};

// -------------------------------

class BehaviorStateReference{
		PROVIDES_TYPE_NAME_NV(BehaviorStateReference)
		Util::Reference<State> state;
	public:
		State * getState()const		{	return state.get();	}
		void setState(State * s)	{	state = s;	}
		//! Return the referenced State or throw an exception if not set.
		MINSGAPI State * requireState()const;
};

}
#endif // BEHAVIOR_STATUS_EXTENSIONS_H
