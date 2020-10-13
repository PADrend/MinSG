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
#ifndef ABSTRACTBEHAVIOURDECORATOR_H_
#define ABSTRACTBEHAVIOURDECORATOR_H_

#include "../../Core/Behaviours/AbstractBehaviour.h"

namespace MinSG {
	
//! @ingroup behaviour
class AbstractBehaviourDecorator : public AbstractNodeBehaviour {
	PROVIDES_TYPE_NAME(AbstractBehaviourDecorator)
public:
	MINSGAPI AbstractBehaviourDecorator(AbstractBehaviour* decorated);
	virtual ~AbstractBehaviourDecorator() {}

	/**
	 * Returns the decorated Behaviour.
	 * @return AbstractBehaviour
	 *
	 * @note The returned Behaviour can be another decorator.
	 */
	AbstractBehaviour* getDecorated() { return decoratedBehaviour.get(); }

	/**
	 * Returns the root of decorated behaviours
	 * @return AbstractBehaviour
	 *
	 * @note When you have decorated decorators like
	 * Behaviour <-- Decorator1 <-- Decorator2 <-- Decorator3
	 * this method returns the Behaviour at the top
	 */
	MINSGAPI AbstractBehaviour* getDecoratedRoot();

	// ---|> AbstractNodeBehaviour
	MINSGAPI void onNodeChanged(Node * oldNode) override;

private:
	Util::Reference<AbstractBehaviour> decoratedBehaviour;
};

}

#endif /* ABSTRACTBEHAVIOURDECORATOR_H_ */
