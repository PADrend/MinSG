/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "BehaviourManager.h"
#include "BehaviorStatusExtensions.h"
#include "../NodeAttributeModifier.h"
#include <Util/Macros.h>
#include <Util/ObjectExtension.h>

namespace MinSG {

//!	[ctor]
BehaviourManager::BehaviourManager() : attrName_behaviorStore( NodeAttributeModifier::create("activeBehaviorStatuses", NodeAttributeModifier::PRIVATE_ATTRIBUTE )){
	//ctor
}

//!	[dtor]
BehaviourManager::~BehaviourManager(){
	clearBehaviours();
	//dtor
}

void BehaviourManager::registerBehaviour(AbstractBehaviour * behaviour){
	if(behaviour == nullptr)
		return;
	Util::Reference<AbstractBehaviour> ref=behaviour;
	removeBehaviour(behaviour);

	if(AbstractNodeBehaviour * nodeBehaviour=dynamic_cast<AbstractNodeBehaviour *>(behaviour)){
		registeredNodeBehaviours.insert( std::pair<Node *,Util::Reference<AbstractNodeBehaviour> >(nodeBehaviour->getNode(),nodeBehaviour) );
	}else if(AbstractStateBehaviour * stateBehaviour=dynamic_cast<AbstractStateBehaviour *>(behaviour)){
		registeredStateBehaviours.insert( std::pair<State *,Util::Reference<AbstractStateBehaviour> >(stateBehaviour->getState(),stateBehaviour) );
	}else{
		WARN("registerBehaviour: Unknown behaviour type ");
	}
}

void BehaviourManager::removeBehaviour(AbstractBehaviour * behaviour){
	if(behaviour == nullptr)
		return;
	if(AbstractNodeBehaviour * nodeBehaviour=dynamic_cast<AbstractNodeBehaviour *>(behaviour)){
		Node * node=nodeBehaviour->getNode();
		for(auto it=registeredNodeBehaviours.find(node);
				it!=registeredNodeBehaviours.end() && it->first == node; ++it){
			if( it->second == nodeBehaviour ){
				behaviour->finalize();
				registeredNodeBehaviours.erase(it);
				break;
			}
		}
	}else if(AbstractStateBehaviour * stateBehaviour=dynamic_cast<AbstractStateBehaviour *>(behaviour)){
		State * state=stateBehaviour->getState();
		for(auto it=registeredStateBehaviours.find(state);
				it!=registeredStateBehaviours.end() && it->first == state; ++it){
			if( it->second == stateBehaviour ){
				behaviour->finalize();
				registeredStateBehaviours.erase(it);
				break;
			}
		}
	}else{
		WARN("removeBehaviour: Unknown behaviour type ");
	}
}

void BehaviourManager::clearBehaviours(){
	for(const auto & nodeBehaviourEntry : registeredNodeBehaviours) {
		nodeBehaviourEntry.second->finalize();
	}
	registeredNodeBehaviours.clear();

	for(const auto & stateBehaviourEntry : registeredStateBehaviours) {
		stateBehaviourEntry.second->finalize();
	}
	registeredStateBehaviours.clear();
	
	for(auto & status : activeBehaviorStatuses)
		status->getBehavior()->finalize(*status.get());
	activeBehaviorStatuses.clear();
}

void BehaviourManager::executeBehaviours(AbstractBehaviour::timestamp_t timeSec){
	behaviourList_t tmp;
	executeBehaviours(timeSec,tmp);
}

void BehaviourManager::executeBehaviours(AbstractBehaviour::timestamp_t timeSec,behaviourList_t & finishedBehaviours){
	for(auto it=registeredNodeBehaviours.begin();it!=registeredNodeBehaviours.end();){
		if( it->second->execute(timeSec)==AbstractBehaviour::CONTINUE ){
			++it;
		}else{ // FINISHED
			finishedBehaviours.push_back( it->second.get() );
			registeredNodeBehaviours.erase(it++);
		}
	}
	for(auto it=registeredStateBehaviours.begin();it!=registeredStateBehaviours.end();){
		if( it->second->execute(timeSec)==AbstractBehaviour::CONTINUE ){
			++it;
		}else{ // FINISHED
			finishedBehaviours.push_back( it->second.get() );
			registeredStateBehaviours.erase(it++);
		}
	}
	// new behaviors
	executeBehaviors(timeSec);
}

BehaviourManager::nodeBehaviourList_t BehaviourManager::getBehavioursByNode(Node * node)const{
	nodeBehaviourList_t behaviours;
	for(auto it=registeredNodeBehaviours.find(node);
				it!=registeredNodeBehaviours.end() && it->first == node; ++it){
		behaviours.push_back( it->second );
	}
	return behaviours;
}

BehaviourManager::stateBehaviourList_t BehaviourManager::getBehavioursByState(State * state)const{
	stateBehaviourList_t behaviours;
	for(auto it=registeredStateBehaviours.find(state);
				it!=registeredStateBehaviours.end() && it->first == state; ++it){
		behaviours.push_back( it->second );
	}
	return behaviours;
}

// ------------------------------
// Behaviors 2
typedef Util::ReferenceAttribute<BehaviorStatus> statusWrapperAttribute_t;

//! (internal) helper
static void filterFinishedStatuses(Util::GenericAttributeList * l){
	l->erase(	std::remove_if( l->begin(), l->end(), 
						[](std::unique_ptr<Util::GenericAttribute>& attr){ 
							auto attr2 = dynamic_cast<statusWrapperAttribute_t*>(attr.get());
							return !attr2 || attr2->ref().get()->isFinished();
						}),	l->end());
}

//! (internal) helper
static void filterAndAttachStatus(Util::AttributeProvider * attrProvider,const Util::StringIdentifier & attr_name,BehaviorStatus * status){
	auto * l = attrProvider->getAttribute<Util::GenericAttributeList>(attr_name);
	if(l){ // filter finished statuses
		filterFinishedStatuses(l);
	}else{
		l = new Util::GenericAttributeList;
		attrProvider->setAttribute(attr_name,l);
	}
	l->push_back(new statusWrapperAttribute_t(status) );
}

//! (internal) helper
static std::vector<BehaviorStatus*> getContainedActiveStatuses(Util::AttributeProvider * attrProvider,const Util::StringIdentifier & attr_name){

	std::vector<BehaviorStatus*>  statuses;
	auto * l = attrProvider->getAttribute<Util::GenericAttributeList>(attr_name);
	if(l){ // filter finished statuses
		filterFinishedStatuses(l);
		for(auto& wrapper : *l){
			statuses.push_back(dynamic_cast<const statusWrapperAttribute_t*>(wrapper.get())->get());
		}
	}
	return statuses;
}


void BehaviourManager::executeBehaviors(Behavior::timestamp_t timeSec){ 	// must allow registration of additional behaviors during execution
	std::vector<Util::Reference<BehaviorStatus>> temp;
	std::swap(temp,activeBehaviorStatuses);
	activeBehaviorStatuses.reserve(temp.size());
	
	for(auto behaviorStatus : temp){
		const auto result = behaviorStatus->getBehavior()->execute(*behaviorStatus.get(),timeSec);
		if(result == Behavior::CONTINUE)
			activeBehaviorStatuses.emplace_back(behaviorStatus);
	}
}

		
std::vector<BehaviorStatus*> BehaviourManager::getActiveBehaviorStatusesByNode(Node* node){
	return getContainedActiveStatuses(node, attrName_behaviorStore);
}
std::vector<BehaviorStatus*> BehaviourManager::getActiveBehaviorStatusesByState(State* state){
	return getContainedActiveStatuses(state, attrName_behaviorStore);
}


BehaviorStatus * BehaviourManager::startNodeBehavior(Behavior * behavior, Node * node){
	Util::Reference<BehaviorStatus> status = behavior->createBehaviorStatus();
	
	// attach Node to status
	auto nodeRef = Util::getObjectExtension<BehaviorNodeReference>(status.get()); 	//! 	\see BehaviorNodeReference
	if(!nodeRef)
		nodeRef = Util::addObjectExtension<BehaviorNodeReference>(status.get());
	nodeRef->setNode(node);
	activeBehaviorStatuses.emplace_back(status);

	// store status as node attribute
	filterAndAttachStatus(node,attrName_behaviorStore,status.get());

	return status.detachAndDecrease();
}

BehaviorStatus * BehaviourManager::startStateBehavior(Behavior * behavior, State * state){
	Util::Reference<BehaviorStatus> status = behavior->createBehaviorStatus();
	
	// attach State to status
	auto stateRef = Util::getObjectExtension<BehaviorStateReference>(status.get()); //! 	\see BehaviorStateReference
	if(!stateRef)
		stateRef = Util::addObjectExtension<BehaviorStateReference>(status.get());
	stateRef->setState(state);

	activeBehaviorStatuses.emplace_back(status);
	filterAndAttachStatus(state,attrName_behaviorStore,status.get());

	return status.detachAndDecrease();
}




}
