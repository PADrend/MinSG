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
#include "SimplePhysics.h"
#include "../../Core/Nodes/Node.h"
#include "../../Core/Behaviours/BehaviorStatusExtensions.h"
#include <Util/Macros.h>
#include <Util/ObjectExtension.h>
#include <random>

namespace MinSG {

//! [ctor]
SimplePhysics::SimplePhysics(Node * node):AbstractNodeBehaviour(node) ,speed(Geometry::Vec3(0,0,0)),lastTime(-1){
	static std::default_random_engine engine;
	int i = std::uniform_int_distribution<int>(0, 49)(engine);
	int j = std::uniform_int_distribution<int>(0, 99)(engine);
	int k = std::uniform_int_distribution<int>(0, 99)(engine);
	speed=Geometry::Vec3( (j-50)/10.0,i/1.0,(k-50)/10.0);
	//ctor
}//! [ctor]
SimplePhysics::SimplePhysics(Node * node,const Geometry::Vec3& v):AbstractNodeBehaviour(node) ,speed(v),lastTime(-1){
}

//! [dtor]
SimplePhysics::~SimplePhysics(){
	//dtor
}

//! ---|> AbstractBehaviour
Behavior::behaviourResult_t SimplePhysics::doExecute(){
	if(!getNode() || !getNode()->hasParent()){
//		WARN("SimplePhysics::doExecute");
		return FINISHED;
	}
	const timestamp_t timeSec = getCurrentTime();

	if(lastTime==-1)
		lastTime=timeSec;
	float duration=timeSec-lastTime;
	lastTime=timeSec;

	Geometry::Vec3 velocity(0,-9.81*duration,0);
	speed+=velocity;
	getNode()->moveRel(speed*duration);
	float y=getNode()->getWorldPosition().getY();
	if(y<0.0){
		getNode()->moveRel(Geometry::Vec3(0,-y,0));
		if(speed.getY()<0 ) {
			speed.reflect(Geometry::Vec3(0,1,0));
			speed*=0.5;
		}
	}
	return CONTINUE;
}


//----------------------------------------------

struct SimplePhysicsData{
		PROVIDES_TYPE_NAME_NV(SimplePhysicsData)
	public:
		Geometry::Vec3 speed;
};

//! ---|> Behavior
void SimplePhysics2::doPrepareBehaviorStatus(BehaviorStatus & status){
	Util::addObjectExtension<BehaviorNodeReference>(&status);									//! \see BehaviorNodeReference
	Util::addObjectExtension<SimplePhysicsData>(&status);										//! \see SimplePhysicsData
}

//! ---|> Behavior
void SimplePhysics2::doBeforeInitialExecute(BehaviorStatus& status){
	auto& data = *Util::requireObjectExtension<SimplePhysicsData>(&status);						//! \see SimplePhysicsData

	// set random initial speed
	static std::default_random_engine engine;
	std::normal_distribution<> d(1.0,0.5);
	data.speed = Geometry::Vec3( 	initialDirection.x() * d(engine),
									initialDirection.y() * d(engine),
									initialDirection.z() * d(engine) );
}

//! ---|> Behavior
Behavior::behaviourResult_t SimplePhysics2::doExecute2(BehaviorStatus& status){
	Node* node = Util::requireObjectExtension<BehaviorNodeReference>(&status)->requireNode();	//! \see BehaviorNodeReference
	if(node->isDestroyed())
		return FINISHED;

	auto data = Util::requireObjectExtension<SimplePhysicsData>(&status);						//! \see SimplePhysicsData
	auto& speed = data->speed;
	
	const auto duration = status.getCurrentTime()-status.getLastTime();

	const Geometry::Vec3 velocity(0,-9.81*duration,0);
	speed+=velocity;
	node->moveRel(speed*duration);
	const float y = node->getWorldPosition().getY();
	if(y<0.0){
		node->moveRel(Geometry::Vec3(0,-y,0));
		if(speed.getY()<0 ) {
			speed.reflect(Geometry::Vec3(0,1,0));
			speed*=0.5;
		}
	}
	return CONTINUE;
}


}
