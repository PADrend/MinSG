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
#ifndef BEHAVIOURGROUP_H_
#define BEHAVIOURGROUP_H_

#include "../../Core/Behaviours/AbstractBehaviour.h"
#include <list>

namespace MinSG {
	
//! @ingroup behaviour
class BehaviourGroup: public AbstractNodeBehaviour {
	PROVIDES_TYPE_NAME(BehaviourGroup)
public:
	typedef std::list<Util::Reference<AbstractBehaviour> > behaviourList_t;

	MINSGAPI BehaviourGroup();
	MINSGAPI virtual ~BehaviourGroup();

	MINSGAPI void addBehaviour(AbstractBehaviour* behaviour);
	MINSGAPI void removeBehaviour(AbstractBehaviour* behaviour);
	MINSGAPI void getBehaviours(behaviourList_t & list);
	MINSGAPI size_t count();

	// ---|> AbstractBehaviour
	MINSGAPI behaviourResult_t doExecute() override;
	// ---|> AbstractNodeBehaviour
	MINSGAPI void onNodeChanged(Node * oldNode) override;

private:
	behaviourList_t behaviours;
};

}

#endif /* BEHAVIOURGROUP_H_ */
