/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "ExporterTools.h"

#include "ExporterContext.h"
#include "CoreNodeExporter.h"
#include "CoreStateExporter.h"
#include "WriterMinSG.h"

#include "../SceneDescription.h"
#include "../SceneManager.h"
#include "../../Core/Nodes/GroupNode.h"
#include "../../Core/Nodes/Node.h"
#include "../../Core/States/State.h"
#include "../../Core/Behaviours/AbstractBehaviour.h"
#include "../../Core/Behaviours/BehaviourManager.h"
#include "../../Core/NodeAttributeModifier.h"
#include "../../Core/RenderingLayer.h"
#include "../../Helper/StdNodeVisitors.h"

#include "../../Ext/SceneManagement/Exporter/ExtNodeExporter.h"
#include "../../Ext/SceneManagement/Exporter/ExtStateExporter.h"
#include "../../Ext/SceneManagement/Exporter/ExtBehaviourExporter.h"

#include <Geometry/Matrix4x4.h>
#include <Geometry/SRT.h>
#include <Geometry/Vec3.h>
#include <Util/GenericAttribute.h>
#include <Util/GenericAttributeSerialization.h>

#include <Util/References.h>
#include <Util/StringIdentifier.h>

#include <list>
#include <ostream>
#include <utility>

using namespace Geometry;
using namespace Util;
namespace MinSG {
namespace SceneManagement {

static std::unordered_map<Util::StringIdentifier,ExporterTools::NodeExport_Fn_t> nodeExporter;
static std::unordered_map<Util::StringIdentifier,ExporterTools::StateExport_Fn_t> stateExporter;
static std::vector<ExporterTools::BehaviourExport_Fn_t> behaviourExporter;


static bool handlerInitialized(){
	static bool once = [](){
		/*!
		 * importer / exporter from CORE have to be called BEFORE those from EXT
		 * otherwise e.g. instances of GeometryNodes get lost
		 */
		//@{
		initCoreNodeExporter();
		initCoreStateExporter();
	//  initCoreBehaviourExporter(); // currently no Behaviours in Core

		initExtNodeExporter();
		initExtStateExporter();
		initExtBehaviourExporter();
		return true;
	}();
	return once;
}

//! (static)
void ExporterTools::finalizeBehaviourDescription(ExporterContext & /*ctxt*/,NodeDescription & description, AbstractBehaviour * behaviour){
	if(behaviour)
		description.setString(Consts::TYPE,Consts::TYPE_BEHAVIOUR);
}

static void addMatrixToDescription(NodeDescription & description, const Geometry::Matrix4x4f & matrix) {
	if(!matrix.isIdentity()) {
		std::ostringstream matrixStream;
		matrixStream << matrix;
		description.setValue(Consts::ATTR_MATRIX, Util::GenericAttribute::createString(matrixStream.str()));
	}
}

//! (static)
void ExporterTools::addSRTToDescription(NodeDescription & description, const Geometry::SRT & srt) {
	const Vec3 posV = srt.getTranslation();
	if (posV!=Vec3(0,0,0)) {
		std::stringstream pos;
		pos<<posV.getX()<<" "<<posV.getY()<<" "<<posV.getZ();
		description.setValue(Consts::ATTR_SRT_POS,GenericAttribute::createString(pos.str()));
	}
	const Vec3 dirV=srt.getDirVector();
	const Vec3 upV=srt.getUpVector();
	if (dirV!=Vec3(0,0,1) || upV!=Vec3(0,1,0)) {
		std::stringstream dir;
		dir<<dirV.getX()<<" "<<dirV.getY()<<" "<<dirV.getZ();
		description.setValue(Consts::ATTR_SRT_DIR,GenericAttribute::createString(dir.str()));

		std::stringstream up;
		up<<upV.getX()<<" "<<upV.getY()<<" "<<upV.getZ();
		description.setValue(Consts::ATTR_SRT_UP,GenericAttribute::createString(up.str()));
	}
	if (srt.getScale() != 1.0) {
		std::stringstream s;
		s<<srt.getScale();
		description.setValue(Consts::ATTR_SRT_SCALE,GenericAttribute::createString(s.str()));
	}
}

void ExporterTools::addTransformationToDescription(NodeDescription & description, Node * node) {
	if(node->hasSRT()) {
		addSRTToDescription(description, node->getSRT());
	} else if(node->hasMatrix()) {
		if(node->getMatrixPtr()->convertsSafelyToSRT()) {
			addSRTToDescription(description, node->getMatrixPtr()->_toSRT());
		} else {
			addMatrixToDescription(description, *node->getMatrixPtr());
		}
	}
}

//! (static)
void ExporterTools::addAttributesToDescription(ExporterContext & ctxt, NodeDescription & description, const Util::GenericAttribute::Map * attribs) {
	if( attribs ) {
		for(auto & attrib : *attribs) {
			const std::string attributeName = attrib.first.toString();
			// Ignore attributes that should not be saved
			if(!NodeAttributeModifier::isSaved(attributeName))
				continue;

			const Util::GenericAttribute * attribute = attrib.second.get();

			std::unique_ptr<NodeDescription> attributeDescription(new NodeDescription);
			attributeDescription->setString(Consts::TYPE, Consts::TYPE_ATTRIBUTE);
			attributeDescription->setString(Consts::ATTR_ATTRIBUTE_NAME, attributeName);

			if(dynamic_cast<const Util::GenericNumberAttribute *>(attribute) != nullptr) {
				if(dynamic_cast<const Util::_NumberAttribute<float> *>(attribute) != nullptr) {
					attributeDescription->setString(Consts::ATTR_ATTRIBUTE_TYPE, Consts::ATTRIBUTE_TYPE_FLOAT);
				} else if(dynamic_cast<const Util::_NumberAttribute<int> *>(attribute) != nullptr) {
					attributeDescription->setString(Consts::ATTR_ATTRIBUTE_TYPE, Consts::ATTRIBUTE_TYPE_INT);
				} else { // unspecific number
					attributeDescription->setString(Consts::ATTR_ATTRIBUTE_TYPE, Consts::ATTRIBUTE_TYPE_NUMBER);
				}
				attributeDescription->setString(Consts::ATTR_ATTRIBUTE_VALUE, attribute->toString());
			} else if(dynamic_cast<const Util::BoolAttribute *>(attribute) != nullptr) {
				attributeDescription->setString(Consts::ATTR_ATTRIBUTE_TYPE, Consts::ATTRIBUTE_TYPE_BOOL);
				attributeDescription->setString(Consts::ATTR_ATTRIBUTE_VALUE, attribute->toString());
			} else if(dynamic_cast<const Util::StringAttribute *>(attribute) != nullptr) {
				attributeDescription->setString(Consts::ATTR_ATTRIBUTE_TYPE, Consts::ATTRIBUTE_TYPE_STRING);
				attributeDescription->setString(Consts::ATTR_ATTRIBUTE_VALUE, attribute->toString());
			} else {
				attributeDescription->setString(Consts::ATTR_ATTRIBUTE_TYPE, Consts::ATTRIBUTE_TYPE_GENERIC);
				static const Util::StringIdentifier CONTEXT_DATA_SCENEMANAGER("SceneManager");
				std::unique_ptr<Util::GenericAttributeMap> context(new Util::GenericAttributeMap);
				context->setValue(CONTEXT_DATA_SCENEMANAGER, new Util::WrapperAttribute<SceneManager &>(ctxt.sceneManager));
				attributeDescription->setString(Consts::ATTR_ATTRIBUTE_VALUE, 
												   Util::GenericAttributeSerialization::serialize(attribute, context.get()));
			}
			addChildEntry(description,std::move(attributeDescription));
		}
	}
}

//! (static)
void ExporterTools::addChildEntry(	NodeDescription & description, std::unique_ptr<NodeDescription> childDescription) {
	NodeDescriptionList * children = dynamic_cast<NodeDescriptionList *> (description.getValue(Consts::CHILDREN));
	if( !children ) {
		children = new NodeDescriptionList;
		description.setValue(Consts::CHILDREN, children);
	}
	children->push_back(childDescription.release());
}

//! (static)
void ExporterTools::addDataEntry(NodeDescription & description,std::unique_ptr<NodeDescription> dataDescription) {
	dataDescription->setString(Consts::TYPE, Consts::TYPE_DATA);
	addChildEntry(description, std::move(dataDescription));
}

//! (static)
void ExporterTools::addChildNodesToDescription(ExporterContext & ctxt,NodeDescription & description, Node * node){
	for(const auto & child : getChildNodes(node)){
		std::unique_ptr<NodeDescription> childDescription(createDescriptionForNode(ctxt, child));
		if(childDescription)
			addChildEntry(description,std::move(childDescription));
	}
}

//! (static)
void ExporterTools::addStatesToDescription(ExporterContext & ctxt,NodeDescription & description,Node * node){
	if( node->hasStates() ){
		for(const auto & state : node->getStates()){
			if(!state->isTempState()){
				std::unique_ptr<NodeDescription> stateDescription(createDescriptionForState(ctxt, state));
				if(stateDescription)
					ExporterTools::addChildEntry(description,std::move(stateDescription));
			}
		}
	}
}

//! (static)
void ExporterTools::addBehavioursToDescription(ExporterContext & ctxt,NodeDescription & description, Node * node){
	BehaviourManager::nodeBehaviourList_t behaviours = ctxt.sceneManager.getBehaviourManager()->getBehavioursByNode(node);
	auto * children = dynamic_cast<NodeDescriptionList *>(description.getValue(Consts::CHILDREN));
	if (!children) {
		children = new NodeDescriptionList;
		description.setValue(Consts::CHILDREN,children);
	}
	for(BehaviourManager::nodeBehaviourList_t::const_iterator it=behaviours.begin();it!=behaviours.end();++it){
		auto behaviourDescription(createDescriptionForBehaviour(ctxt,it->get()));
		if(behaviourDescription){
			behaviourDescription->setString(Consts::TYPE,Consts::TYPE_BEHAVIOUR);
			children->push_back(behaviourDescription.release());
		}
	}
}

static void removeTempNodeId(ExporterContext & ctxt, const std::string & nodeId){
	ctxt.sceneManager.unregisterNode(nodeId);
}

std::unique_ptr<NodeDescription> ExporterTools::createDescriptionForNode(ExporterContext & ctxt,Node * node){
	if(!node || node->isTempNode() || !handlerInitialized())
		return nullptr;

	std::unique_ptr<NodeDescription> description(new NodeDescription);
	
	if(node->isInstance()){
		Node * prototype = node->getPrototype();
		std::string prototypeId = ctxt.sceneManager.getNameOfRegisteredNode(prototype);

		// if the node has no id, create a temporary one.
		if(prototypeId.empty()){
			do{
				std::ostringstream name;
				name << "_tmp_"<< (ctxt.tmpNodeCounter++);
				prototypeId = name.str();
			}while( ctxt.sceneManager.getRegisteredNode(prototypeId)!=nullptr );
			ctxt.sceneManager.registerNode(prototypeId,prototype);
			ctxt.addFinalizingAction(std::bind(removeTempNodeId, std::placeholders::_1, prototypeId));
		}
		description->setString(Consts::ATTR_NODE_TYPE,Consts::NODE_TYPE_CLONE);   // set the node-type to "clone",
		description->setString(Consts::ATTR_CLONE_SOURCE,prototypeId);  // set a reference to the prototype-node,
		if(!ctxt.isPrototypeUsed(prototypeId)) {   // add used prototype to usedPrototype-list
			const bool b = ctxt.creatingDefinitions;
			ctxt.creatingDefinitions = true;
			std::unique_ptr<NodeDescription> newPrototypeDescription( createDescriptionForNode(ctxt,prototype) );
			if(newPrototypeDescription){
				ctxt.addUsedPrototype(prototypeId,std::move(newPrototypeDescription));
			}
			ctxt.creatingDefinitions = b;
		}
	}else{
		auto handler = nodeExporter.find(node->getTypeId());
		if(handler==nodeExporter.end()){
			WARN(std::string("Unsupported node type ") + node->getTypeName());
			return nullptr;
		}
		handler->second(ctxt,*description,node);
		ExporterTools::addStatesToDescription(ctxt,*description,node);
		if(node->hasFixedBB()){
			const Geometry::Box& bb = node->getBB();
			const Geometry::Vec3 center = bb.getCenter();
			std::stringstream s;
			s << center.getX() << " " << center.getY() << " " << center.getZ() << " " << bb.getExtentX() << " " << bb.getExtentY() << " " << bb.getExtentZ();
			description->setString(Consts::ATTR_FIXED_BB, s.str());
		}
		
	}
		
	// finalize node description
	description->setString(Consts::TYPE,Consts::TYPE_NODE);
	if(dynamic_cast<GroupNode*>(node) && node->isClosed())
		description->setString(Consts::ATTR_FLAG_CLOSED, "true");

	addTransformationToDescription(*description, node);
	addAttributesToDescription(ctxt,*description, node->getAttributes());
	addBehavioursToDescription(ctxt,*description,node);

	if(node->getRenderingLayers()!=RENDERING_LAYER_DEFAULT)
		description->setValue(Consts::ATTR_RENDERING_LAYERS, Util::GenericAttribute::createNumber<renderingLayerMask_t>(node->getRenderingLayers()));
	
	// registered name(=id) of the node
	const std::string nodeId = ctxt.sceneManager.getNameOfRegisteredNode(node);
	if(!nodeId.empty()) { // set id (if set)
		description->setString(Consts::ATTR_NODE_ID, nodeId);
	}
	return description;
}

std::unique_ptr<NodeDescription> ExporterTools::createDescriptionForScene(ExporterContext & ctxt, const std::deque<Node *> &nodes) {
	std::unique_ptr<NodeDescription> sceneDescription( new NodeDescription );
	sceneDescription->setValue(Consts::TYPE, Util::GenericAttribute::createString(Consts::TYPE_SCENE));
	sceneDescription->setValue(Consts::ATTR_SCENE_VERSION, Util::GenericAttribute::createString("2.9"));

	// Create the descriptions for the nodes (and collect all prototypes that are used)
	std::deque<std::unique_ptr<NodeDescription>> nodeDescriptions;
	for(const auto & node : nodes) {
		auto nodeDescription( createDescriptionForNode(ctxt, node) );
		if(nodeDescription == nullptr) {
			WARN("Can't create description for node.");
			continue;
		}
		nodeDescriptions.emplace_back(nodeDescription.release());
	}

		// create description for defs-section (prototypes)
	if(!ctxt.usedPrototypes.empty()) {
		std::unique_ptr<NodeDescription> prototypesDescription(new NodeDescription);
		prototypesDescription->setValue(Consts::TYPE, Util::GenericAttribute::createString("defs"));

		for(auto & desc : ctxt.usedPrototypes){
			addChildEntry(*prototypesDescription,std::move(desc));
		}
		ctxt.usedPrototypes.clear();
		
		// first come the definitions
		addChildEntry(*sceneDescription,std::move(prototypesDescription) );
	}

	// finally add the node descriptions
	for(auto & nodeDesciption : nodeDescriptions)
		addChildEntry(*sceneDescription,std::move(nodeDesciption) );
	
	ctxt.executeFinalizingActions();

	return sceneDescription;
}

std::unique_ptr<NodeDescription> ExporterTools::createDescriptionForState(ExporterContext & ctxt,State * state) {
	if( !state || !handlerInitialized())
		return nullptr;
	
	std::unique_ptr<NodeDescription> description(new NodeDescription);
	description->setString(Consts::TYPE, Consts::TYPE_STATE);
	
	// registered name(=id) of the state
	const Util::StringIdentifier stateId = ctxt.sceneManager.getStateId(state);
	if(!stateId.empty()){
		auto it = ctxt.usedStateIds.find(stateId);
		if(it!=ctxt.usedStateIds.end()){ // has the state already been exported?
			description->setString(Consts::ATTR_STATE_TYPE,  Consts::STATE_TYPE_REFERENCE);
			description->setString(Consts::ATTR_REFERENCED_STATE_ID,  stateId.toString());
				
			if( ctxt.creatingDefinitions && !it->second.second ){ // if this state part of a prototype, but it has already been stored at a regular node ->
				// swap the original description with this referencing description. This prevents that a reference may be placed before the declaration.
				NodeDescription tmp( std::move( *it->second.first ));
				*it->second.first = std::move(*description);
				*description = std::move(tmp);

//				WARN("Swapping state description: "+stateId.toString());
				ctxt.usedStateIds[stateId] = std::make_pair(description.get(),true);
			}
			return description;
		}
	}

	const auto handler = stateExporter.find(state->getTypeId());
	if(handler==stateExporter.end()){
		WARN(std::string("Unsupported State type ") + state->getTypeName());
		return nullptr;
	}
	handler->second(ctxt,*description,state);

	// finalize description
	addAttributesToDescription(ctxt,*description.get(),state->getAttributes());

	if(state->getRenderingLayers()!=RENDERING_LAYER_DEFAULT)
		description->setValue(Consts::ATTR_RENDERING_LAYERS, Util::GenericAttribute::createNumber<renderingLayerMask_t>(state->getRenderingLayers()));

	if(!stateId.empty()) { // set id (if set)
		description->setString(Consts::ATTR_STATE_ID, stateId.toString()); // id=$stateId
		ctxt.usedStateIds.emplace( stateId, std::make_pair(description.get(),ctxt.creatingDefinitions) );
	}

	return description;
}

std::unique_ptr<NodeDescription> ExporterTools::createDescriptionForBehaviour(ExporterContext & ctxt,AbstractBehaviour * behaviour) {
	if( !behaviour || !handlerInitialized())
		return nullptr;

	NodeDescription * desc = nullptr;
	for(const auto & exporter : behaviourExporter) {
		desc = exporter(ctxt,behaviour);
		if(desc != nullptr)
			break;
	}
	if(desc == nullptr) {
		WARN(std::string("Unsupported Behaviour type ") + behaviour->getTypeName());
		return nullptr;
	}
	return std::unique_ptr<NodeDescription>(desc);
}

void ExporterTools::registerNodeExporter(const Util::StringIdentifier & classId,NodeExport_Fn_t fn) {
	nodeExporter[classId] = fn;
}
void ExporterTools::registerStateExporter(const Util::StringIdentifier & classId,StateExport_Fn_t fn) {
	stateExporter[classId] = fn;
}
void ExporterTools::registerBehaviourExporter(BehaviourExport_Fn_t fn) {
	behaviourExporter.push_back(fn);
}



}
}
