/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef BEHAVIOR_H
#define BEHAVIOR_H
#include <Util/ReferenceCounter.h>
#include <Util/TypeNameMacro.h>
#include "BehaviorStatus.h"

namespace MinSG {

//! @defgroup behaviour Behaviours

/**
	Behavior base class.
	
	To start a behavior for an object (without BehaviorManager):
	\code
		Util::Reference<Behavior> b = new SpecificBehavior; // Create Behavior object (can be used for several objects)
		auto status = b->createBehaviorStatus();			// Prepare BehaviorStatus for animated object 
		Util::requireObjectExtension<BehaviorMyObjectReference>(status)->setMyObject(myObject); // start behavior for the object
		
		while(!status.isFinished()){ // execute until finished
				status->getBehavior()->execute( *status.get(), getCurrentTime() );
				// ...
		}
	\endcode
	
	\note Use a BehaviorManager for managing behavior statuses.
	@ingroup behaviour

**/
class Behavior :
			public Util::ReferenceCounter<Behavior>,
			public Util::AttributeProvider {
		PROVIDES_TYPE_NAME(Behavior)

	public:
		typedef double timestamp_t; // time in seconds
		enum behaviourResult_t{		CONTINUE, FINISHED	};

		//! (ctor)
		Behavior() = default;

		//! (dtor)
		virtual ~Behavior() {}

		MINSGAPI Util::Reference<BehaviorStatus> createBehaviorStatus();
		MINSGAPI behaviourResult_t execute(BehaviorStatus & state,timestamp_t currentTimeSec);
		
		/*! Finalize the given BehaviorState.
			\note An already finalized state is silently ignored.	*/
		MINSGAPI void finalize(BehaviorStatus & state);
		
	private:
		/*! ---o
			Called by createBehaviorStatus() with the newly created BehaviorStatus object.
			This is the place to add extensions to the BehaviorStatus (Util::addObjectExtension<...>(...)).	*/
		virtual void doPrepareBehaviorStatus(BehaviorStatus &)		{}
		/*! ---o
			Called by execute(...) once for each BehaviorStatus object.	*/
		virtual void doBeforeInitialExecute(BehaviorStatus &)		{}
		/*! ---o
			Called by execute(...).
			\return CONTINUE or FINISHED. If FINISHED is returned, finalize(state) is called.	*/
		virtual behaviourResult_t doExecute2(BehaviorStatus &)		{	return CONTINUE;	}	//! \todo ---> doExecute(...)
		/*! ---o
			Called by finalize(state) to clean up the given @p status.	*/
		virtual void doFinalize(BehaviorStatus &)					{}
};

}
#endif // BEHAVIOR_H
