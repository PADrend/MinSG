/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "ImporterTools.h"


#include "../../Core/States/State.h"
#include "../../Core/Nodes/GroupNode.h"
#include "../../Core/Nodes/ListNode.h"
#include "../../Core/Nodes/Node.h"
#include "../SceneDescription.h"
#include "../SceneManager.h"


#include "ReaderMinSG.h"
#include "ReaderDAE.h"
#include "MeshImportHandler.h"
#include "ImportContext.h"
#include "CoreNodeImporter.h"
#include "CoreStateImporter.h"

#include "../../Ext/SceneManagement/Importer/ExtAdditionalDataImporter.h"
#include "../../Ext/SceneManagement/Importer/ExtNodeImporter.h"
#include "../../Ext/SceneManagement/Importer/ExtStateImporter.h"
#include "../../Ext/SceneManagement/Importer/ExtBehaviourImporter.h"

#include <Geometry/Matrix4x4.h>
#include <Geometry/SRT.h>
#include <Geometry/Vec3.h>
#include <Util/IO/FileName.h>
#include <Util/IO/FileUtils.h>
#include <Util/AttributeProvider.h>
#include <Util/GenericAttribute.h>
#include <Util/GenericAttributeSerialization.h>
#include <Util/JSON_Parser.h>
#include <Util/Macros.h>
#include <Util/StringUtils.h>
#include <Util/Utils.h>
#include <Util/Timer.h>

#include <iosfwd>
#include <vector>

namespace MinSG {
namespace SceneManagement {
namespace ImporterTools {

// ---

static bool handlerInitialized(){
	static bool once = [](){
		/*!
		 * importer / exporter from CORE have to be called BEFORE those from EXT
		 * otherwise e.g. instances of GeometryNodes get lost
		 */
		//@{


	// 	initCoreAdditionalDataImporter(); // currently no AdditionalData in Core
		initCoreNodeImporter();
		initCoreStateImporter();
	//  initCoreBehaviourImporter(); // currently no Behaviours in Core

		initExtAdditionalDataImporter();
		initExtNodeImporter();
		initExtStateImporter();
		initExtBehaviourImporter();
		return true;
	}();
	return once;
}

std::vector<NodeImport_Fn_t> nodeImporter;
std::vector<StateImport_Fn_t> stateImporter;
std::vector<BehaviourImport_Fn_t> behaviourImporter;
std::vector<AdditionalDataImport_Fn_t> additionalDataImporter;
	
void registerNodeImporter(NodeImport_Fn_t fn) 						{	nodeImporter.push_back(fn);	}
void registerStateImporter(StateImport_Fn_t fn)						{	stateImporter.push_back(fn);	}
void registerBehaviourImporter(BehaviourImport_Fn_t fn)				{	behaviourImporter.push_back(fn);	}
void registerAdditionalDataImporter(AdditionalDataImport_Fn_t fn)	{	additionalDataImporter.push_back(fn);	}


static bool processDescription(ImportContext & ctxt, const NodeDescription & d, Node * parent) {
	if(parent == nullptr){
		WARN("parent may not be null");
		return false;
	}
	if(!handlerInitialized())
		FAIL();

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


static void removeTempNodeId(ImportContext & ctxt, const std::string & nodeId){
	ctxt.sceneManager.unregisterNode(nodeId);
}

//!	If the given @a description contains a node identifier, the given @a node is registered with that identifier in the local SceneManager.
static void registerNamedNode(ImportContext & ctxt,const NodeDescription & description, Node * node)  {
	static const std::string tmpPrefix("_tmp_");
	const std::string id = description.getString(Consts::ATTR_NODE_ID);
	if (!id.empty()) {
		ctxt.sceneManager.registerNode(id, node);
		if(id.compare(0,tmpPrefix.size(),tmpPrefix)==0){
			ctxt.addFinalizingAction(std::bind(removeTempNodeId, std::placeholders::_1, id));
		}
	}
}

//! If the given @a description contains a state identifier, the given @a state is registered with that identifier in the local SceneManager.
static void registerNamedState(ImportContext & ctxt,const NodeDescription & description, State * state)  {
	const std::string id = description.getString(Consts::ATTR_STATE_ID);
	if (!id.empty()) {
		ctxt.sceneManager.registerState(id, state);
	}
}

//! (static)
void finalizeNode(ImportContext & ctxt, Node * node,const NodeDescription & d){
	if(node==nullptr)
		return;
	registerNamedNode(ctxt,d, node);
	{ // applyTransformation(d, node);
		Util::GenericAttribute * matrixAttribute = d.getValue(Consts::ATTR_MATRIX);
		node->reset();
		if (matrixAttribute != nullptr) {
			std::istringstream matrixStream(matrixAttribute->toString());
			Geometry::Matrix4x4f matrix;
			matrixStream >> matrix;
			if (!matrix.isIdentity()) {
				node->setMatrix(matrix);
			}
		} else {
			Geometry::SRT srt = getSRT(d);
			if (srt.getScale() != 1.0f || !srt.getRotation().isIdentity() || !srt.getTranslation().isZero()) {
				node->setSRT(srt);
			}
		}
	}
	if(dynamic_cast<GroupNode*>(node)){
		if(d.contains(Consts::ATTR_FLAG_CLOSED) && d.getString(Consts::ATTR_FLAG_CLOSED)=="true")
			node->setClosed(true);
	}

	if(d.contains(Consts::ATTR_FIXED_BB)){
		const std::string bbString = d.getString(SceneManagement::Consts::ATTR_FIXED_BB);
		const auto boxValues = Util::StringUtils::toFloats(bbString);
		FAIL_IF(boxValues.size() != 6);
		node->setFixedBB(Geometry::Box(Geometry::Vec3(boxValues[0], boxValues[1], boxValues[2]), boxValues[3], boxValues[4], boxValues[5]));
	}

	const NodeDescriptionList * subDescriptions = dynamic_cast<const NodeDescriptionList *> (d.getValue(Consts::CHILDREN));
	if(subDescriptions==nullptr)
		return;

	GroupNode * group=dynamic_cast<GroupNode *>(node);
	// add states, behaviours and children
	for (auto & subDescription : *subDescriptions) {
		const NodeDescription * desc = dynamic_cast<const NodeDescription *> (subDescription.get());
		if (desc == nullptr) {
			FAIL();
		}

		const std::string type = desc->getString(Consts::TYPE);
		if (type == Consts::TYPE_STATE || type == Consts::TYPE_BEHAVIOUR ) {
			if (!processDescription(ctxt,*desc, node)) {
				WARN("Creating State failed.");
				continue;
			}
		}else if (type == Consts::TYPE_NODE) {
			if (group == nullptr) {
				WARN("Only GroupNodes can have children.");
				continue;
			}
			if (!processDescription(ctxt,*desc, group)) {
				WARN("Creation of child node failed.");
				continue;
			}
		}
	}

	// Add the attributes *after* the children have been added to make sure the children have already been registered at the scene manager.
	addAttributes(ctxt, subDescriptions, node);
}

//! (static)
void finalizeState(ImportContext & ctxt, State * state,const NodeDescription & d){
	if(state==nullptr)
		return;
	registerNamedState(ctxt,d, state);
	const NodeDescriptionList * subDescriptions = dynamic_cast<const NodeDescriptionList *> (d.getValue(Consts::CHILDREN));
	addAttributes(ctxt, subDescriptions, state);
}

//! (static)
Geometry::SRT getSRT(const NodeDescription & d) {
	Geometry::Vec3 pos(0,0,0);
	{
		Util::GenericAttribute * attr = d.getValue(Consts::ATTR_SRT_POS);
		if (attr != nullptr) {
			const auto values = Util::StringUtils::toFloats(attr->toString());
			if (values.size() != 3) {
				WARN("Syntax error in position of SRT.");
				return Geometry::SRT();
			}
			pos.setValue(values[0],values[1],values[2]);
		}
	}
	Geometry::Vec3 dir(0,0,1);
	{
		Util::GenericAttribute * attr = d.getValue(Consts::ATTR_SRT_DIR);
		if (attr != nullptr) {
			const auto values = Util::StringUtils::toFloats(attr->toString());
			if (values.size() != 3) {
				WARN("Syntax error in direction vector of SRT.");
				return Geometry::SRT();
			}
			dir.setValue(values[0],values[1],values[2]);
		}
	}
	Geometry::Vec3 up(0,1,0);
	{
		Util::GenericAttribute * attr = d.getValue(Consts::ATTR_SRT_UP);
		if (attr != nullptr) {
			const auto values = Util::StringUtils::toFloats(attr->toString());
			if (values.size() != 3) {
				WARN("Syntax error in up vector of SRT.");
				return Geometry::SRT();
			}
			up.setValue(values[0],values[1],values[2]);
		}
	}
	float scale = 1.0f;
	{
		Util::GenericAttribute * srtScale = d.getValue(Consts::ATTR_SRT_SCALE);
		if (srtScale != nullptr) {
			scale = Util::StringUtils::toNumber<float>(srtScale->toString());
		}
	}
	return Geometry::SRT(pos,dir,up,scale);
}

//! (static)
void addAttributes(ImportContext & ctxt, const NodeDescriptionList * subDescriptions, Util::AttributeProvider * attrProvider)  {
	if (subDescriptions == nullptr) {
		return;
	}
	for (auto & subDescription : *subDescriptions) {
		const NodeDescription * desc = dynamic_cast<const NodeDescription *> (subDescription.get());
		if (desc == nullptr) {
			FAIL();
		}
		const std::string type = desc->getString(Consts::TYPE);
		if (type == Consts::TYPE_ATTRIBUTE) {
			const std::string name = desc->getString(Consts::ATTR_ATTRIBUTE_NAME, "");
			const std::string attrType = desc->getString(Consts::ATTR_ATTRIBUTE_TYPE, Consts::ATTRIBUTE_TYPE_STRING);
			std::string value = desc->getString(Consts::ATTR_ATTRIBUTE_VALUE, "");
			if(value.empty()){ // if the value is not set as value="...", try to use the data block.
				Util::GenericAttribute * dataBlock = desc->getValue(Consts::DATA_BLOCK);
				if(dataBlock!=nullptr){
					value = dataBlock->toString();
				}
			}
			Util::GenericAttribute * attr = nullptr;
			if (!name.empty() && !value.empty()) {
				// Compare with strings used in ExporterTools::addAttributesToDescription
				if (attrType == Consts::ATTRIBUTE_TYPE_FLOAT) {
					attr = Util::GenericAttribute::createNumber(Util::StringUtils::toNumber<float>(value));
				} else if (attrType == Consts::ATTRIBUTE_TYPE_INT) {
					attr = Util::GenericAttribute::createNumber(Util::StringUtils::toNumber<int>(value));
				} else if (attrType == Consts::ATTRIBUTE_TYPE_BOOL) {
					attr = Util::GenericAttribute::createBool(Util::StringUtils::toBool(value));
				} else if (attrType == Consts::ATTRIBUTE_TYPE_STRING || attrType == "String") { // 'String' is deprecated!
					attr = Util::GenericAttribute::createString( value );
				} // unspecific number
				else if (attrType == Consts::ATTRIBUTE_TYPE_NUMBER) {
					if (value.find('.') == std::string::npos) {
						attr = Util::GenericAttribute::createNumber(Util::StringUtils::toNumber<int>(value));
					} else {
						attr = Util::GenericAttribute::createNumber(Util::StringUtils::toNumber<float>(value));
					}
				}else if (attrType == Consts::ATTRIBUTE_TYPE_JSON) {
					attr = Util::JSON_Parser::parse(value);
				}else if (attrType == Consts::ATTRIBUTE_TYPE_GENERIC) {
					static const Util::StringIdentifier CONTEXT_DATA_SCENEMANAGER("SceneManager");
					std::unique_ptr<Util::GenericAttributeMap> context(new Util::GenericAttributeMap);
					context->setValue(CONTEXT_DATA_SCENEMANAGER, new Util::WrapperAttribute<SceneManager &>(ctxt.sceneManager));
					attr = Util::GenericAttributeSerialization::unserialize(value, context.get());
				}else {
					WARN(std::string("Unable to import an attribute of unsupported type: '")+attrType+'\'');
					attr = Util::GenericAttribute::createString(value);
				}
			}
			if (attr != nullptr) {
				attrProvider->setAttribute(name, attr);
			}
		}
	}
}

//! (static)
std::deque<const NodeDescription *> filterElements(const std::string & type, 
												   const NodeDescriptionList * subDescriptions) {
	std::deque<const NodeDescription *> result;
	if (subDescriptions == nullptr) {
		return result;
	}
	for (auto & subDescription : *subDescriptions) {
		const NodeDescription * desc = dynamic_cast<const NodeDescription *> (subDescription.get());
		if (desc == nullptr) {
			FAIL();
		}
		const std::string elementType = desc->getString(Consts::TYPE);
		if (elementType == type) {
			result.push_back(desc);
		}
	}
	return result;
}


//! (static)
Util::FileName checkRelativePaths(const ImportContext & ctxt,const Util::FileName & fileName) {
	Util::FileName newName;
	std::list<std::string> hints;
	hints.push_back(ctxt.getFileName().getFSName() + "://" + ctxt.getFileName().getDir());
	hints.push_back("");
	if (Util::FileUtils::findFile(fileName, hints, newName)) {
		return newName;
	} else {
		WARN(std::string("Cannot find file \"") + fileName.toString() + "\".");
		return fileName;
	}

}





//! The import handler for meshes is called whenever a mesh has to be created.
static std::unique_ptr<MeshImportHandler> meshImportHandler(new MeshImportHandler);
MeshImportHandler * getMeshImportHandler() 							{	return meshImportHandler.get();	}

//! @note This object takes ownership of the import handler and will delete it when it is not needed anymore.
void setMeshImportHandler(std::unique_ptr<MeshImportHandler> handler)	{	meshImportHandler=std::move(handler);	}

void buildSceneFromDescription(ImportContext & ctxt,const NodeDescription * d) {
	Util::info << "\nBegin parsing description:\n";
	if(d == nullptr) {
		WARN("No scene description!");
		return;
	}
	if(!ctxt.getRootNode()) {
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
						processDescription(ctxt, *p, dummy.get());
						//Util::info << "Created Prototype \""<<getNameOfRegisteredNode(n)<<"\".\n";
					} else if(p->getString(Consts::TYPE) == Consts::TYPE_ADDITIONAL_DATA) {
						// handleAdditionalData(ctxt, *p);
						bool handled = false;
						for(const auto & importer : additionalDataImporter) {
							if(importer(ctxt, p->getString(Consts::ATTR_NODE_TYPE), *p)) {
								handled = true;
								break;
							}
						}
						if(!handled)
							WARN("Could not handle additional data Node!");
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
				if(!processDescription(ctxt, *e, ctxt.getRootNode())) {
					WARN("Could not create Node");
				}
			} else {
				WARN(std::string("Unsupported Type:")+e->getString(Consts::TYPE));
			}
		}
	}

	/// finalize
	ctxt.executeFinalizingActions();
}
	
}
}
}
