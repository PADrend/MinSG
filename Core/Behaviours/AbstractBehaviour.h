/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef ABSTRACTBEHAVIOUR_H
#define ABSTRACTBEHAVIOUR_H

#include "Behavior.h"

namespace MinSG {
class Node;
class State;

/**
 *  AbstractBehaviour ---|> Behavior   (Compatibility class)
 * \deprecated do not use for new Behaviors!
 * @ingroup behaviour
 */
class AbstractBehaviour : public Behavior {
		PROVIDES_TYPE_NAME(AbstractBehaviour)

	protected:
		Util::Reference<BehaviorStatus> myStatus;

	public:

		MINSGAPI AbstractBehaviour();
		virtual ~AbstractBehaviour() {}

		timestamp_t getLastTime()const				{	return myStatus->getLastTime();	}
		timestamp_t getCurrentTime()const			{	return myStatus->getCurrentTime();	}
		timestamp_t getTimeDelta()const				{	return getCurrentTime() - getLastTime();	}
		
		behaviourResult_t execute(timestamp_t currentTimeSec)	{	return Behavior::execute(*myStatus.get(),currentTimeSec);	}
		
		void finalize()								{	return Behavior::finalize(*myStatus.get());	}
	private:

		/*! ---o
			Called every frame (if the behaviour is active).	*/
		virtual behaviourResult_t doExecute()=0;

		/*! ---o
			Called once before doExecute is executed for the first time. */
		virtual void onInit()						{}
	
	private:
		//! ---|> Behavior
		virtual void doBeforeInitialExecute(BehaviorStatus &) override	{ 	onInit();	}
		//! ---|> Behavior
		virtual behaviourResult_t doExecute2(BehaviorStatus &) override	{	return doExecute();	}
};

/***
 **  AbstractNodeBehaviour ---|> AbstractBehaviour
 **/
class AbstractNodeBehaviour : public AbstractBehaviour {
		PROVIDES_TYPE_NAME(AbstractNodeBehaviour)
	public:

		MINSGAPI AbstractNodeBehaviour(Node * node);
		virtual ~AbstractNodeBehaviour()	{}

		MINSGAPI Node * getNode()const;

		/**
		 * Sets the node of the AbstractNodeBehaviour.
		 * @param node
		 *
		 * @note You should really know what you are doing when using this method,
		 * because it can result in unpredictable behaviour.
		 *
		 * @note This method calls onNodeChanged(oldNode) to allow cleanup when needed.
		 */
		MINSGAPI void setNode(Node * newNode);
		
		/// ---o
		virtual void onNodeChanged(Node * /*oldNode*/) {}
};

/***
 **  AbstractStateBehaviour ---|> AbstractBehaviour
 **/
class AbstractStateBehaviour : public AbstractBehaviour {
		PROVIDES_TYPE_NAME(AbstractNodeBehaviour)
	public:

		MINSGAPI AbstractStateBehaviour(State * state);
		virtual ~AbstractStateBehaviour()	{}

		MINSGAPI State * getState()const;
};

}
#endif // ABSTRACTBEHAVIOUR_H
