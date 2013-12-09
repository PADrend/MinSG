/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "SceneManager.h"

#include "../Core/Nodes/GeometryNode.h"
#include "../Core/States/State.h"
#include "../Core/Behaviours/BehaviourManager.h"
#include "../Helper/StdNodeVisitors.h"

namespace MinSG {
namespace SceneManagement {

template<class Obj>
class TreeRegistry{
		std::unordered_map<Util::StringIdentifier,Util::Reference<Obj>> map_idToObj;
		std::unordered_map<Obj*,Util::StringIdentifier> map_objToId;
	public:
		TreeRegistry() {}
		~TreeRegistry() = default;

		Obj * get(const Util::StringIdentifier & id)const{
			const auto it = map_idToObj.find(id);
			return it==map_idToObj.end() ? nullptr : it->second.get();
		}
		Util::StringIdentifier getId(Obj * obj)const{
			const auto it = map_objToId.find(obj);
			return it==map_objToId.end() ? Util::StringIdentifier() : it->second;
		}
		std::vector<Util::StringIdentifier> getIds()const{
			std::vector<Util::StringIdentifier> ids;
			for(const auto & entry : map_idToObj)
				ids.emplace_back(entry.first);
			return ids;
		}

		bool hasId(Obj * obj)const			{	return !getId(obj).empty();	}
		void setId(Util::Reference<Obj> obj,const Util::StringIdentifier & id){
			if(!id.empty()) // remove a obj possibly previously registered with the id
				removeId( id );
			if(obj.isNotNull()) // remove the obj's old id
				removeId( obj.get() ); 
			if(obj.isNotNull()&&!id.empty()){ // register the obj using the new id
				map_objToId.emplace(obj.get(),id);
				map_idToObj.emplace(id, std::move(obj));
			}
		}

		Util::StringIdentifier createAndSetId(Util::Reference<Obj> obj){
			Util::StringIdentifier id = getId(obj.get());
			if(obj.isNotNull() && id.empty()) {
				do { // Create a new, random identifier....
					id = Util::StringIdentifier("$" + Util::StringUtils::createRandomString(6));
				}while( get(id) ); // ... until an unused one is found.
				setId(obj,id);
			}
			return id;
		}
		void removeId(Obj * obj){
			auto it = map_objToId.find(obj);
			if(it!=map_objToId.end()){
				map_idToObj.erase(it->second);
				map_objToId.erase(it);
			}
		}
		void removeId(const Util::StringIdentifier & id) {
			auto it = map_idToObj.find(id);
			if(it!=map_idToObj.end()){
				map_objToId.erase(it->second.get());
				map_idToObj.erase(it);
			}
		}
};
//--------------------

//! [ctor]
SceneManager::SceneManager() : Util::AttributeProvider(),
		nodeRegistry(new TreeRegistry<Node>),stateRegistry(new TreeRegistry<State>) {
}

//! [dtor]
SceneManager::~SceneManager() = default;

// ----------- Node Registration

void SceneManager::registerNode(const Util::StringIdentifier & id, Util::Reference<Node> node) {
	nodeRegistry->setId(node,id);
}

void SceneManager::registerNode(Util::Reference<Node>  node) {
	nodeRegistry->createAndSetId(node);
}

void SceneManager::unregisterNode(const Util::StringIdentifier & id) {
	nodeRegistry->removeId(id);
}

Node * SceneManager::getRegisteredNode(const Util::StringIdentifier & id) const {
	return nodeRegistry->get(id);
}

std::vector<Util::StringIdentifier> SceneManager::getNodeIds()const{
	return nodeRegistry->getIds();
}

void SceneManager::getNamesOfRegisteredNodes(std::vector<std::string> & ids) const {
	for(const auto & id : nodeRegistry->getIds() )
		ids.emplace_back(id.toString());
}

Util::StringIdentifier SceneManager::getNodeId(Node * node) const {
	return nodeRegistry->getId(node);
}

Node * SceneManager::createInstance(const Util::StringIdentifier & id) const {
	const Node * prototype = getRegisteredNode(id);
	return prototype ? Node::createInstance(prototype) : nullptr;
}

void SceneManager::registerGeometryNodes(Node * rootNode) {
	forEachNodeTopDown<GeometryNode>(rootNode, [this](Node* node){registerNode(node);});
}

// ----------- registered States

void SceneManager::registerState(const Util::StringIdentifier & id, Util::Reference<State> state) {
	stateRegistry->setId(state,id);
}

void SceneManager::registerState(Util::Reference<State>  state) {
	stateRegistry->createAndSetId(state);
}

void SceneManager::unregisterState(const Util::StringIdentifier & id) {
	stateRegistry->removeId(id);
}

State * SceneManager::getRegisteredState(const Util::StringIdentifier & id) const {
	return stateRegistry->get(id);
}

std::vector<Util::StringIdentifier> SceneManager::getStateIds()const{
	return stateRegistry->getIds();
}

void SceneManager::getNamesOfRegisteredStates(std::vector<std::string> & ids) const {
	for(const auto & id : stateRegistry->getIds() )
		ids.emplace_back(id.toString());
}

Util::StringIdentifier SceneManager::getStateId(State * state) const {
	return stateRegistry->getId(state);
}

// -----------  registered Behaviours
BehaviourManager * SceneManager::getBehaviourManager() {
	if(behaviourManager.isNull())
		behaviourManager = new BehaviourManager;
	return behaviourManager.get();
}

const BehaviourManager * SceneManager::getBehaviourManager()const {
	if(behaviourManager.isNull())
		behaviourManager = new BehaviourManager;
	return behaviourManager.get();
}

}
}
