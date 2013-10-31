/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "GroupNode.h"
#include "../../Helper/StdNodeVisitors.h"

namespace MinSG {

GroupNode::GroupNode():Node() {
	setClosed(false);
	//ctor
}

void GroupNode::addChild(Util::Reference<Node> child){
	if(child.isNotNull()){
		child->removeFromParent();
		doAddChild(child);
		invalidateCompoundBB();
		worldBBChanged();
		Node::informNodeAddedObservers(child.get());
	}
}

bool GroupNode::removeChild(Util::Reference<Node> child){
	if( child.isNotNull() && doRemoveChild(child) ){
		invalidateCompoundBB();
		worldBBChanged();
		Node::informNodeRemovedObservers(this,child.get());
		return true;
	}
	return false;
}

void GroupNode::clearChildren(){
	for(auto & child : MinSG::getChildNodes(this)){
		const Util::Reference<Node> childHolder( child );
		this->removeChild(childHolder);
	}
}

}
