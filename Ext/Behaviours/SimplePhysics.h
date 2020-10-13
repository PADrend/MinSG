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
#ifndef SIMPLEPHYSICS_H
#define SIMPLEPHYSICS_H

#include "../../Core/Behaviours/AbstractBehaviour.h"
#include <Geometry/Vec3.h>

namespace MinSG {
/**
 * SimplePhysics ---|> AbstractNodeBehaviour
 * @ingroup behaviour
 */
class SimplePhysics : public AbstractNodeBehaviour {
	PROVIDES_TYPE_NAME(SimplePhysics)

	public:
		MINSGAPI SimplePhysics(Node * node,Geometry::Vec3 v);
		MINSGAPI SimplePhysics(Node * node);
		MINSGAPI virtual ~SimplePhysics();

		MINSGAPI behaviourResult_t doExecute() override;

	protected:
		Geometry::Vec3 speed;
		float lastTime;
};

/**
 * SimplePhysics ---|> Behavior
 * @ingroup behaviour
 */
class SimplePhysics2 : public Behavior {
	PROVIDES_TYPE_NAME(SimplePhysics)

	public:
		SimplePhysics2(Geometry::Vec3 v) : initialDirection(std::move(v)) {}
		SimplePhysics2() : initialDirection(0,1,0) {}
		virtual ~SimplePhysics2() {}
		
	private:
		Geometry::Vec3 initialDirection;

		//! ---|> Behavior
		MINSGAPI void doPrepareBehaviorStatus(BehaviorStatus &) override;
		//! ---|> Behavior
		MINSGAPI void doBeforeInitialExecute(BehaviorStatus &) override;
		//! ---|> Behavior
		MINSGAPI behaviourResult_t doExecute2(BehaviorStatus &) override;
//		//! ---|> Behavior
//		void doFinalize(BehaviorStatus &)override;
};

}
#endif // SIMPLEPHYSICS_H
