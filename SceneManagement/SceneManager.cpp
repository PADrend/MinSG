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

#include "SceneDescription.h"

#include "Exporter/ExporterContext.h"
#include "Exporter/ExporterTools.h"
#include "Exporter/CoreNodeExporter.h"
#include "Exporter/CoreStateExporter.h"

#include "Importer/ImportContext.h"
#include "Importer/CoreNodeImporter.h"
#include "Importer/CoreStateImporter.h"

#include "../Ext/SceneManagement/Exporter/ExtNodeExporter.h"
#include "../Ext/SceneManagement/Exporter/ExtStateExporter.h"
#include "../Ext/SceneManagement/Exporter/ExtBehaviourExporter.h"

#include "../Ext/SceneManagement/Importer/ExtAdditionalDataImporter.h"
#include "../Ext/SceneManagement/Importer/ExtNodeImporter.h"
#include "../Ext/SceneManagement/Importer/ExtStateImporter.h"
#include "../Ext/SceneManagement/Importer/ExtBehaviourImporter.h"

#include "ReaderMinSG.h"
#include "WriterMinSG.h"
#include "ReaderDAE.h"

#include "../Core/Nodes/Node.h"
#include "../Core/Nodes/GeometryNode.h"
#include "../Core/Nodes/GroupNode.h"
#include "../Core/Nodes/ListNode.h"
#include "../Core/States/State.h"
#include "../Core/States/ShaderState.h"
#include "../Core/States/LightingState.h"
#include "../Core/Behaviours/BehaviourManager.h"
#include "../Helper/StdNodeVisitors.h"

#include <Rendering/Mesh/Mesh.h>
#include <Rendering/MeshUtils/MeshUtils.h>
#include <Rendering/Serialization/Serialization.h>
#include <Rendering/Serialization/GenericAttributeSerialization.h>
#include <Rendering/Texture/Texture.h>

#include <Util/IO/FileName.h>
#include <Util/IO/FileUtils.h>
#include <Util/AttributeProvider.h>
#include <Util/Macros.h>
#include <Util/References.h>
#include <Util/StringUtils.h>
#include <Util/Timer.h>
#include <Util/Utils.h>
#include <Util/Macros.h>

#include <functional>
#include <memory>

#ifdef MINSG_EXT_LOADERCOLLADA
#include "../Ext/LoaderCOLLADA/LoaderCOLLADA.h"
#endif

namespace MinSG {
namespace SceneManagement {

//! [ctor]
SceneManager::SceneManager() : Util::AttributeProvider(), meshImportHandler(new MeshImportHandler) {

	/*!
	 * importer / exporter from CORE have to be called BEFORE those from EXT
	 * otherwise e.g. instances of GeometryNodes get lost
	 */
	//@{
	initCoreNodeExporter(*this);
	initCoreStateExporter(*this);
//  initCoreBehaviourExporter(*this); // currently no Behaviours in Core

// 	initCoreAdditionalDataImporter(*this); // currently no AdditionalData in Core
	initCoreNodeImporter(*this);
	initCoreStateImporter(*this);
//  initCoreBehaviourImporter(*this); // currently no Behaviours in Core

	initExtNodeExporter(*this);
	initExtStateExporter(*this);
	initExtBehaviourExporter(*this);

	initExtAdditionalDataImporter(*this);
	initExtNodeImporter(*this);
	initExtStateImporter(*this);
	initExtBehaviourImporter(*this);
	//@}
}

//! [dtor]
SceneManager::~SceneManager() {
}

// ----------- Node Registration

void SceneManager::registerNode(const Util::StringIdentifier & name, Node * node) {
	const Util::StringIdentifier id(name);
	if(node == nullptr) {  // no node? -> unregister nodes with that name
		nodeRegistry.eraseLeft(id);
		return;
	}
	// Store a temporary reference to the new Node.
	Util::Reference<Node> ref = node;

	// Check if the same Node is already registered...
	auto idIt = nodeRegistry.findRight(node);
	if(idIt!=nodeRegistry.endRight()) {
		if(id==idIt->second) {
			// ... by the same name -> do nothing.
			return;
		} else {
			// ... by another name -> remove the node's old entry
			nodeRegistry.eraseRight(node);
		}
	}

	// remove the node previously registered by id.
	nodeRegistry.eraseLeft(id);
	nodeRegistry.insert(id,node);
}

void SceneManager::registerNode(Node * node) {
	if(node == nullptr || isNodeRegistered(node)) {  // no node or already registered?
		return;
	}
	std::string newId(7, '$');
	do {
		// Create a new, random identifier.
		newId.replace(1, 6, Util::StringUtils::createRandomString(6));
		// Make sure we get a unique identifier.
	} while(getRegisteredNode(newId) != nullptr);
	registerNode(newId, node);
}

void SceneManager::unregisterNode(const Util::StringIdentifier & name) {
	registerNode(name, nullptr);
}

Node * SceneManager::getRegisteredNode(const Util::StringIdentifier & name) const {
	const nodeRegistry_t::const_iterator_left it = nodeRegistry.findLeft(name);
	if(it == nodeRegistry.endLeft())
		return nullptr;
	return it->second.get();
}

void SceneManager::getNamesOfRegisteredNodes(std::vector<std::string> & names) const {
	for(auto it = nodeRegistry.beginLeft(); it != nodeRegistry.endLeft(); ++it) {
		names.push_back(it->first.toString());
	}
}

std::string SceneManager::getNameOfRegisteredNode(Node * node) const {
	return getNameIdOfRegisteredNode(node).toString();
}

Util::StringIdentifier SceneManager::getNameIdOfRegisteredNode(Node * node) const {
	Util::StringIdentifier nodeId;
	if(node == nullptr)
		return nodeId;
	const auto it = nodeRegistry.findRight(node);
	if( it != nodeRegistry.endRight() )
		nodeId = it->second;
	return nodeId;
}



Node * SceneManager::createInstance(const Util::StringIdentifier & name) const {
	const Node * prototype = getRegisteredNode(name);
	return prototype ? Node::createInstance(prototype) : nullptr;
}

void SceneManager::registerGeometryNodes(Node * rootNode) {
	forEachNodeTopDown<GeometryNode>(rootNode, std::bind(std::mem_fn(static_cast<void (SceneManager::*)(Node *)>(&SceneManager::registerNode)), this, std::placeholders::_1));
}

// ----------- registered States

void SceneManager::registerState(const std::string & name, State * state) {
	const Util::StringIdentifier id(name);
	if(state == nullptr) {  // no state? -> unregister states with that name
		stateRegistry.eraseLeft(id);
		return;
	}
	// Store a temporary reference to the new State.
	Util::Reference<State> ref = state;

	// Check if the same State is already registered...
	auto idIt = stateRegistry.findRight(state);
	if(idIt!=stateRegistry.endRight()) {
		if(id==idIt->second) {
			// ... by the same name -> do nothing.
			return;
		} else {
			// ... by another name -> remove the state's old entry
			stateRegistry.eraseRight(state);
		}
	}

	// remove the state previously registered by id.
	stateRegistry.eraseLeft(id);
	stateRegistry.insert(id,state);
}

void SceneManager::registerState(State * state) {
	if(state == nullptr || !getNameOfRegisteredState(state).empty()) {  // no state or already registered?
		return;
	}
	std::string newId;
	do {
		// Create a new, random identifier.
		newId = '$'+Util::StringUtils::createRandomString(6);
		// Make sure we get a unique identifier.
	} while(getRegisteredState(newId) != nullptr);
	registerState(newId, state);
}

void SceneManager::unregisterState(const std::string & name) {
	registerState(name, nullptr);
}

State * SceneManager::getRegisteredState(const std::string & name) const {
	const stateRegistry_t::const_iterator_left it = stateRegistry.findLeft(name);
	return it == stateRegistry.endLeft() ? nullptr : it->second.get();
}



void SceneManager::getNamesOfRegisteredStates(std::vector<std::string> & names) const {
	for(auto it = stateRegistry.beginLeft(); it != stateRegistry.endLeft(); ++it)
		names.push_back(it->first.toString());
}


std::string SceneManager::getNameOfRegisteredState(State * state) const {
	if(state == nullptr)
		return std::string();
	const stateRegistry_t::const_iterator_right it = stateRegistry.findRight(state);
	return it == stateRegistry.endRight() ?
		   std::string() :
		   it->second.toString();
}

// --------- misc

void SceneManager::saveMeshesInSubtreeAsPLY(Node * rootNode, const std::string & s_dirName, bool saveRegisteredNodes/*=false*/)const {
	float start = Util::Timer::now();
	Util::FileName dirName = Util::FileName::createDirName(s_dirName);
	const auto geoNodes = collectNodes<GeometryNode>(rootNode);
	const size_t numMeshes = geoNodes.size();

	if(!geoNodes.empty() && !Util::FileUtils::isDir(dirName)) {
		Util::FileUtils::createDir(dirName);
	}
	size_t counter = 0;
	size_t successCounter = 0;
	for(const auto & geoNode : geoNodes) {
		counter++;
		Rendering::Mesh * mesh = geoNode->getMesh();

		// skip node, if node has no mesh or mesh already has a corresponding file.
		if(mesh == nullptr || (!(mesh->getFileName().empty()) && !saveRegisteredNodes)) {
			continue;
		}

		Util::FileName fileName = Util::FileUtils::generateNewRandFilename(dirName, "mesh_", ".ply", 8);

		Util::info << "\rExporting " << counter << "/" << numMeshes << "     ";
		if(!Util::FileUtils::isFile(fileName)) {
			bool successful = Rendering::Serialization::saveMesh(mesh, fileName);
			if(successful) {
				successCounter++;
				mesh->setFileName(fileName);
			}
		}
	}
	Util::info << "\nExported " << successCounter << "/" << numMeshes << " in " << (Util::Timer::now() - start) << " sec.\n";
}

void SceneManager::saveMeshesInSubtreeAsMMF(Node * rootNode, const std::string & s_dirName, bool saveRegisteredNodes/*=false*/)const {
	float start = Util::Timer::now();
	Util::FileName dirName = Util::FileName::createDirName(s_dirName);
	const auto geoNodes = collectNodes<GeometryNode>(rootNode);
	const size_t numMeshes = geoNodes.size();

	if(!geoNodes.empty() && !Util::FileUtils::isDir(dirName)) {
		Util::FileUtils::createDir(dirName);
	}
	size_t counter = 0;
	size_t successCounter = 0;
	for(const auto & geoNode : geoNodes) {
		counter++;
		Rendering::Mesh * mesh = geoNode->getMesh();

		// skip node, if node has no mesh or mesh already has a corresponding file.
		if(mesh == nullptr || (!(mesh->getFileName().empty()) && !saveRegisteredNodes))
			continue;

		Util::FileName fileName = Util::FileUtils::generateNewRandFilename(dirName, "mesh_", ".mmf", 8);

		Util::info << "\rExporting " << counter << "/" << numMeshes << "     ";
		if(!Util::FileUtils::isFile(fileName)) {
			bool successful = Rendering::Serialization::saveMesh(mesh, fileName);
			if(successful) {
				successCounter++;
				mesh->setFileName(fileName);
			}
		}
	}
	Util::info << "\nExported " << successCounter << "/" << numMeshes << " in " << (Util::Timer::now() - start) << " sec.\n";
}

// ------------ Factories
void SceneManager::addNodeImporter(NodeImport_Fn_t fn) {
	nodeImporter.push_back(fn);
}
void SceneManager::addStateImporter(StateImport_Fn_t fn) {
	stateImporter.push_back(fn);
}
void SceneManager::addBehaviourImporter(BehaviourImport_Fn_t fn) {
	behaviourImporter.push_back(fn);
}
void SceneManager::addAdditionalDataImporter(AdditionalDataImport_Fn_t fn) {
	additionalDataImporter.push_back(fn);
}

void SceneManager::buildSceneFromDescription(ImportContext & importContext,const NodeDescription * d) {
	Util::info << "\nBegin parsing description:\n";
	if(d == nullptr) {
		WARN("No scene description!");
		return;
	}
	if(!importContext.getRootNode()) {
		WARN("No container!");
		return;
	}

	if(d->getString(Consts::TYPE) != "scene") {
		WARN("Unknown Format.");
		return;
	}

	{
		/// read Definitions
		//		Util::info << "defs:\n";
		auto defs = dynamic_cast<const NodeDescription *>(d->getValue(Consts::DEFINITIONS));
		if(defs) {
			/// prototype Nodes
			auto nodes = dynamic_cast<const NodeDescriptionList *>(defs->getValue(Consts::CHILDREN));
			if(nodes) {
				for(auto & node : *nodes) {
					auto p = dynamic_cast<const NodeDescription *>(node.get());
					if(!p)
						FAIL();
					if(p->getString(Consts::TYPE) == Consts::TYPE_NODE) {
						Util::Reference<ListNode> dummy = new ListNode;
						processDescription(importContext, *p, dummy.get());
						//Util::info << "Created Prototype \""<<getNameOfRegisteredNode(n)<<"\".\n";
					} else if(p->getString(Consts::TYPE) == Consts::TYPE_ADDITIONAL_DATA) {
						handleAdditionalData(importContext, *p);
					} else {
						WARN(std::string("Unsupported Prototype: ") + p->getString(Consts::TYPE));
					}
				}
			}
		}
		Util::info << "---\n";
	}

	{
		/// read children
		auto children = dynamic_cast<const NodeDescriptionList *>(d->getValue(Consts::CHILDREN));
		if(children == nullptr) {
			return;
		}
		for(auto & elem : *children) {
			auto e = dynamic_cast<const NodeDescription *>(elem.get());
			if(e == nullptr) {
				FAIL();
			}

			if(e->getString(Consts::TYPE) == Consts::TYPE_NODE) {
				if(!processDescription(importContext, *e, importContext.getRootNode())) {
					WARN("Could not create Node");
				}
			} else {
				WARN(std::string("Unsupported Type:")+e->getString(Consts::TYPE));
			}
		}
	}

	/// finalize
	importContext.executeFinalizingActions();
}

bool SceneManager::processDescription(ImportContext & ctxt, const NodeDescription & d, Node * parent) {

	if(parent == nullptr){
		WARN("parent may not be null");
		return false;
	}

	const std::string type = d.getString(Consts::TYPE);

	if(type == Consts::TYPE_NODE) {
		GroupNode * group = dynamic_cast<GroupNode *>(parent);
		if(group==nullptr){
			WARN(std::string("parent has to be a GroupNode, but given is a ") + parent->getTypeName() + ".");
			return false;
		}
		const std::string subType = d.getString(Consts::ATTR_NODE_TYPE);
		for(const auto & importer : nodeImporter) {
			if(importer(ctxt, subType, d, group))
				return true;
		}
		WARN(subType + " could not be imported, no importer found");

	} else if(type == Consts::TYPE_STATE) {
		const std::string subType = d.getString(Consts::ATTR_STATE_TYPE);
		for(const auto & importer : stateImporter) {
			if(importer(ctxt, subType, d, parent))
				return true;
		}
		WARN(subType + " could not be imported, no importer found");

	} else if(type == Consts::TYPE_BEHAVIOUR) {
		const std::string subType = d.getString(Consts::ATTR_BEHAVIOUR_TYPE);
		for(const auto & importer : behaviourImporter) {
			if(importer(ctxt, subType, d, parent))
				return true;;
		}
		WARN(subType + " could not be imported, no importer found");

	} else {
		WARN("the roof is on fire!");
	}

	return false;
}

void SceneManager::handleAdditionalData(ImportContext & ctxt, const NodeDescription & d) {
	for(const auto & importer : additionalDataImporter) {
		if(importer(ctxt, d.getString(Consts::ATTR_NODE_TYPE), d)) {
			return;
		}
	}
	WARN("Could not handle additional data Node!");
}

std::deque<Util::Reference<Node>> SceneManager::loadMinSGFile(const Util::FileName & fileName, const importOption_t importOptions/*=IMPORT_OPTION_NONE*/) {
	auto importContext = createImportContext(importOptions);
	return loadMinSGFile(importContext, fileName);
}

std::deque<Util::Reference<Node>> SceneManager::loadMinSGFile(ImportContext & importContext,const Util::FileName & fileName) {
	importContext.setFileName(fileName);

	std::unique_ptr<std::istream> in(Util::FileUtils::openForReading(fileName));
	if(!in) {
		WARN(std::string("Could not load file: ") + fileName.toString());
		return std::deque<Util::Reference<Node>>();
	}

	return loadMinSGStream(importContext, *(in.get()));
}

std::deque<Util::Reference<Node>> SceneManager::loadMinSGStream(ImportContext & importContext, std::istream & in) {
	if(!in.good()) {
		WARN("Cannot load MinSG nodes from the given stream.");
		return std::deque<Util::Reference<Node>>();
	}

	// parse xml and create description
	std::unique_ptr<const NodeDescription> sceneDescription(ReaderMinSG::loadScene(in));

	// create MinSG scene tree from description with dummy root node
	Util::Reference<ListNode> dummyContainerNode=new ListNode;
	importContext.setRootNode(dummyContainerNode.get());

	buildSceneFromDescription(importContext, sceneDescription.get());

	// detach nodes from dummy root node
	std::deque<Util::Reference<Node>> nodes;
	const auto nodesTmp = getChildNodes(dummyContainerNode.get());
	for(const auto & node : nodesTmp) {
		nodes.push_back(node);
		node->removeFromParent();
	}
	importContext.setRootNode(nullptr);
	return nodes;
}

ImportContext SceneManager::createImportContext(const importOption_t importOptions) {
	return {*this, nullptr, importOptions, Util::FileName("")};
}

GroupNode * SceneManager::loadCOLLADA(const Util::FileName & fileName, const importOption_t importOptions) {
	auto importContext = createImportContext(importOptions);
	return loadCOLLADA(importContext, fileName);
}

GroupNode * SceneManager::loadCOLLADA(ImportContext & importContext, const Util::FileName & fileName) {
	Util::Reference<GroupNode> container = new ListNode;
	importContext.setFileName(fileName);
	importContext.setRootNode(container.get());

	Util::Timer timer;
	timer.reset();
    
	const bool invertTransparency = (importContext.getImportOptions() & IMPORT_OPTION_DAE_INVERT_TRANSPARENCY) > 0;
#ifdef MINSG_EXT_LOADERCOLLADA
    const NodeDescription * sceneDescription = LoaderCOLLADA::loadScene(fileName, invertTransparency);
#else  
    std::unique_ptr<std::istream> in(Util::FileUtils::openForReading(fileName));
	if(!in) {
		WARN(std::string("Could not load file: ") + fileName.toString());
		return nullptr;
	}

    std::unique_ptr<const NodeDescription> sceneDescriptionPtr(ReaderDAE::loadScene(*(in.get()), invertTransparency));
	auto sceneDescription = sceneDescriptionPtr.get();
#endif
    
    Util::info << timer.getSeconds() << "s";
	timer.reset();

	Util::info << "\nBuilding scene graph...";
	buildSceneFromDescription(importContext, sceneDescription);

	Util::info << timer.getSeconds() << "s";
	Util::info << "\tdone.\n";

	importContext.setRootNode(nullptr);

	return container.detachAndDecrease();
}

// -------------- Export  std::function<NodeDescription *(ExporterContext & ctxt,Node * node)
void SceneManager::addNodeExporter(const Util::StringIdentifier & classId,NodeExport_Fn_t fn) {
	nodeExporter[classId] = fn;
}
void SceneManager::addStateExporter(const Util::StringIdentifier & classId,StateExport_Fn_t fn) {
	stateExporter[classId] = fn;
}
void SceneManager::addBehaviourExporter(BehaviourExport_Fn_t fn) {
	behaviourExporter.push_back(fn);
}

NodeDescription * SceneManager::createDescriptionForScene(ExporterContext & ctxt, const std::deque<Node *> &nodes) {
	std::unique_ptr<NodeDescription> sceneDescription( new NodeDescription );
	sceneDescription->setValue(Consts::TYPE, Util::GenericAttribute::createString(Consts::TYPE_SCENE));
	sceneDescription->setValue(Consts::ATTR_SCENE_VERSION, Util::GenericAttribute::createString("2.9"));

	// Create the descriptions for the nodes (and collect all prototypes that are used)
	std::deque<std::unique_ptr<NodeDescription>> nodeDescriptions;
	for(const auto & node : nodes) {
		NodeDescription * nodeDescription = createDescriptionForNode(ctxt, node);
		if(nodeDescription == nullptr) {
			WARN("Can't create description for node.");
			continue;
		}
		nodeDescriptions.emplace_back(nodeDescription);
	}

	// create description for defs-section (prototypes)
	if(!ctxt.usedPrototypes.empty()) {
		NodeDescription prototypesDescription;
		prototypesDescription.setValue(Consts::TYPE, Util::GenericAttribute::createString("defs"));

		for(const auto & desc : ctxt.usedPrototypes){
			std::unique_ptr<NodeDescription> prototypeDesc( desc.clone() );
			ExporterTools::addChildEntry(prototypesDescription,std::move(*prototypeDesc));
		}
		// first come the definitions
		ExporterTools::addChildEntry(*sceneDescription,std::move(prototypesDescription) );
	}

	// finally add the node descriptions
	for(auto & nodeDesciption : nodeDescriptions)
		ExporterTools::addChildEntry(*sceneDescription,std::move(*nodeDesciption) );
	
	ctxt.executeFinalizingActions();

	return sceneDescription.release();
}

bool SceneManager::saveMinSGFile(const Util::FileName & fileName, const std::deque<Node *> & nodes) {
	// generate output
	std::unique_ptr<std::ostream> out(Util::FileUtils::openForWriting(fileName));
	if(out.get() == nullptr){
		WARN(std::string("Cannot write to file ") + fileName.toString());
		return false;
	}

	ExporterContext ctxt(*this);
	ctxt.sceneFile = fileName;
	return saveMinSGStream(ctxt, *(out.get()), nodes);
}

bool SceneManager::saveMinSGStream(ExporterContext & exportContext, std::ostream & out, const std::deque<Node *> & nodes) {
	if(!out.good()) {
		WARN("Cannot load MinSG nodes from the given stream.");
		return false;
	}
	std::unique_ptr<NodeDescription> description(createDescriptionForScene(exportContext, nodes));
	return WriterMinSG::save(out, *(description.get()));
}

//! (internal)
static void removeTempNodeId(ExporterContext & ctxt, const std::string & nodeId){
	ctxt.sceneManager.unregisterNode(nodeId);
}

NodeDescription * SceneManager::createDescriptionForNode(ExporterContext & ctxt,Node * node)const {
	if(!node || node->isTempNode())
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
			std::unique_ptr<NodeDescription> newPrototypeDescription( ctxt.sceneManager.createDescriptionForNode(ctxt,prototype) );
			if(newPrototypeDescription){
				ctxt.addUsedPrototype(prototypeId,std::move(*newPrototypeDescription));
			}
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

	ExporterTools::addTransformationToDescription(*description, node);
	ExporterTools::addAttributesToDescription(ctxt,*description, node->getAttributes());
	ExporterTools::addBehavioursToDescription(ctxt,*description,node);

	// registered name(=id) of the node
	std::string nodeId = getNameOfRegisteredNode(node);
	if(!nodeId.empty()) { // set id (if set)
		description->setString(Consts::ATTR_NODE_ID, nodeId);
	}
	return description.release();
}

NodeDescription * SceneManager::createDescriptionForState(ExporterContext & ctxt,State * state) const {
	if(state == nullptr)
		return nullptr;
	
	std::unique_ptr<NodeDescription> description(new NodeDescription);
	description->setString(Consts::TYPE, Consts::TYPE_STATE);
	
	// registered name(=id) of the state
	const std::string stateId = getNameOfRegisteredState(state);
	if(!stateId.empty() && ctxt.usedStateIds.count(stateId)!=0) { // has the state already been exported? Then use a reference here.
		// <state type="reference" refId=$stateId />
		description->setString(Consts::ATTR_STATE_TYPE,  Consts::STATE_TYPE_REFERENCE);
		description->setString(Consts::ATTR_REFERENCED_STATE_ID,  stateId);
		return description.release();
	}

	auto handler = stateExporter.find(state->getTypeId());
	if(handler==stateExporter.end()){
		WARN(std::string("Unsupported State type ") + state->getTypeName());
		return nullptr;
	}
	handler->second(ctxt,*description,state);

	// finalize description
	ExporterTools::addAttributesToDescription(ctxt,*description.get(),state->getAttributes());

	if(!stateId.empty()) { // set id (if set)
		description->setString(Consts::ATTR_STATE_ID, stateId); // id=$stateId
		ctxt.usedStateIds.insert(stateId);
	}

	return description.release();
}

NodeDescription * SceneManager::createDescriptionForBehaviour(ExporterContext & ctxt,AbstractBehaviour * behaviour) const {
	if(behaviour == nullptr)
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
	return desc;
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
